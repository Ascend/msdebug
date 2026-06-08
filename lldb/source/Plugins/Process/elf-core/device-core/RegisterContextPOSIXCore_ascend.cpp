/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "RegisterContextPOSIXCore_ascend.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend910B.h"
#include "lldb/Target/RegisterContext.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend310P.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend950.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/RegisterValue.h"
#include <lldb/Utility/LLDBLog.h>
using namespace lldb_private;
using namespace device_core;

std::unique_ptr<RegisterContextPOSIXCore_ascend>
RegisterContextPOSIXCore_ascend::Create(Thread &thread, const ArchSpec &arch,
                                        SocType soc_type) {
  Log *log = GetLog(LLDBLog::Thread);
  std::unique_ptr<RegisterInfoPOSIX_ascend> register_info;
  if (soc_type == SocType::ASCEND910B) {
    register_info = std::make_unique<RegisterInfoPOSIXCore_ascend910B>(arch);
  } else if (soc_type == SocType::ASCEND950) {
    register_info = std::make_unique<RegisterInfoPOSIXCore_ascend950>(arch);
  }
  if (!register_info) {
    LLDB_LOG(log, "Create register info failed for soc_type={0}.", static_cast<uint32_t>(soc_type));
  }
  return std::unique_ptr<RegisterContextPOSIXCore_ascend>(
      new RegisterContextPOSIXCore_ascend(thread, std::move(register_info)));
}

RegisterContextPOSIXCore_ascend::~RegisterContextPOSIXCore_ascend() = default;

RegisterContextPOSIXCore_ascend::RegisterContextPOSIXCore_ascend(
    Thread &thread, std::unique_ptr<RegisterInfoPOSIX_ascend> register_info)
    : RegisterContext(thread, 0),
      m_register_info(std::move(register_info)) {}

// Extract mask0 bits from reg_value, and replace mask1 bits in pc with those bits.
inline void ReplacePCBit(uint32_t reg_value, uint64_t mask0, uint64_t mask1,
                         uint64_t &pc) {
  Log *log = GetLog(LLDBLog::Thread);
  if (__builtin_popcountll(mask0) != __builtin_popcountll(mask1)) {
    LLDB_LOG(log, "Internel error: invalid mask, stop replacing, please check err_info_reg map table define.");
    return;
  }
  uint64_t pc0 = reg_value & mask0;
  uint64_t l0 = __builtin_ctzll(mask0);
  uint64_t l1 = __builtin_ctzll(mask1);
  if (l0 > l1) {
    pc0 >>= (l0 - l1);
  } else {
    pc0 <<= (l1 - l0);
  }
  pc = (pc & ~(1ULL * mask1)) | pc0;
}

void RegisterContextPOSIXCore_ascend::FixPCByErrorInfoReg(
    const ErrRegMask &err_map, uint64_t &pc) {
  Log *log = GetLog(LLDBLog::Thread);
  // hit one kind of xx_err_info register
  for (const auto &info_reg_mask: err_map.err_info_regs) {
    const RegisterInfo *reg_info = GetRegisterInfoAtIndex(info_reg_mask.reg_num);
    RegisterValue err_info_reg_value;
    if (!ReadRegister(reg_info, err_info_reg_value)) {
      continue;
    }
    uint32_t err_info_u32_value = err_info_reg_value.GetAsUInt32();
    for (const auto &p: info_reg_mask.pc_mask_map) {
      LLDB_LOG(log, "Start replace pc bit with err_info_value={0:x}, mask0={1:x}, mask1={2:x}, pc={3:x}",
               err_info_u32_value, p.first, p.second, pc);
      ReplacePCBit(err_info_u32_value, p.first, p.second, pc);
    }
  }
}

bool RegisterContextPOSIXCore_ascend::CheckAicErrorRegisterIsValid(
    size_t num_registers, const vector<vector<ErrRegMask>> *error_table,
    const RegisterSet *const reg_set, ErrRegMask &err_reg_mask) {
  uint32_t reg_num = 0;
  Log *log = GetLog(LLDBLog::Thread);
  for (size_t reg_idx = 0; reg_idx < num_registers; ++reg_idx) {
    const uint32_t reg = reg_set->registers[reg_idx];
    const RegisterInfo *reg_info = GetRegisterInfoAtIndex(reg);
    RegisterValue err_reg_value;
    if (!ReadRegister(reg_info, err_reg_value)) {
      continue;
    }
    uint32_t err_u32_value = err_reg_value.GetAsUInt32();
    LLDB_LOG(log, "Got {0} register value={1:x}.", reg_info->name,
             err_u32_value);
    for (const auto &err_map: (*error_table)[reg_idx]) {
      if ((err_u32_value & err_map.mask) == 0)
        continue;
      if (err_map.err_info_regs[0].reg_num != reg_num) {
        if (reg_num == 0) {
          reg_num = err_map.err_info_regs[0].reg_num;
          err_reg_mask = err_map;
          return true;
        } else {
          return false;
        }
      }
    }
  }
  return reg_num != 0;
}

string RegisterContextPOSIXCore_ascend::GetStopErrorRegister() {
  Log *log = GetLog(LLDBLog::Thread);
  const vector<vector<ErrRegMask>> *error_table =
      m_register_info->GetAicErrorRegTable();
  if (!error_table) {
    LLDB_LOG(log, "Got empty error register table.");
    return "";
  }
  auto register_map = m_register_info->GetRegisterMap();
  auto num_register_sets = m_register_info->GetRegisterSetCount();
  for (size_t set_idx = 0; set_idx < num_register_sets; ++set_idx) {
    const RegisterSet *const reg_set = m_register_info->GetRegisterSet(set_idx);
    if (string(reg_set->short_name) != "aic_err_reg") {
      continue;
    }
    const size_t num_registers = reg_set->num_registers;
    LLDB_LOG_ONCE(log, "Got aic_err_reg set, num={0}.", num_registers);
    ErrRegMask err_reg_mask;
    if (!CheckAicErrorRegisterIsValid(num_registers, error_table, reg_set, err_reg_mask)) {
      LLDB_LOG(log, "There are different error type in AIC_ERROR_X register"
              "or no AIC_ERROR_X register value, stop fixing pc.");
      return "";
    }
    const RegisterInfo *reg_info = GetRegisterInfoAtIndex(err_reg_mask.err_info_regs[0].reg_num);
    return reg_info->name;
  }
  return "";
}

// Fix pc value by aic error register info
// For example:
// AIC_ERROR_1=0x40000000, pc=0x12c04120395c
// 30th bit is vec_ub_wrap_around, so we read VEC_ERR_INFO_0 and VEC_ERR_INFO_1
// VEC_ERR_INFO_0=0xfca8 and VEC_ERR_INFO_1=0xf
// We need to use [0,7] bit of 0xfca8 -> (0xa8) replace [2,9] bit of PC:
//    err_info_0 value       =  10101000
// last 12 bit of origin_pc  =100101011100
// so got last 12 bit of pc  =101010100000
// got pc = 0x12c041203aa0
// We need to use [0,7] bit of 0xf -> (0xf) replace [10,17] bit of PC:
//    err_info_1 value           =  00001111
// last [8-20] bit of origin_pc  =000000111010
// so got last 12 bit of pc      =000000111110
// finally got pc = 0x12c041203ea0
void RegisterContextPOSIXCore_ascend::FixPC(uint64_t &pc) {
  Log *log = GetLog(LLDBLog::Thread);
  const vector<vector<ErrRegMask>> *error_table =
      m_register_info->GetAicErrorRegTable();
  if (!error_table) {
    LLDB_LOG(log, "Got empty error register table.");
    return;
  }
  auto register_map = m_register_info->GetRegisterMap();
  auto num_register_sets = m_register_info->GetRegisterSetCount();
  for (size_t set_idx = 0; set_idx < num_register_sets; ++set_idx) {
    const RegisterSet *const reg_set = m_register_info->GetRegisterSet(set_idx);
    if (string(reg_set->short_name) != "aic_err_reg") {
      continue;
    }
    LLDB_LOG(log, "Got aic_err_reg set.");
    const size_t num_registers = reg_set->num_registers;
    ErrRegMask err_reg_mask{};
    if (!CheckAicErrorRegisterIsValid(num_registers, error_table, reg_set, err_reg_mask)) {
      LLDB_LOG(log, "There are different error type in AIC_ERROR_X register, stop fixing pc");
      return;
    }
    uint64_t new_pc = pc;
    FixPCByErrorInfoReg(err_reg_mask, new_pc);
    LLDB_LOG(log, "Got new_pc={0:x}, old_pc={1:x}", new_pc, pc);
    pc = new_pc;
  }
}

Status RegisterContextPOSIXCore_ascend::ReadRegister(
    uint64_t addr, const RegisterInfo *reg_info, uint32_t core_id,
    CoreType core_type, RegisterValue &value) const {
  Status error;
  const auto& summary_info = *m_summary_info;
  const auto data_index = summary_info.focus_pos_info.core_id;
  if (summary_info.reg_data.find(data_index) == summary_info.reg_data.end() ||
      summary_info.reg_data.at(data_index).find(addr) ==
      summary_info.reg_data.at(data_index).end()) {
      error.SetErrorStringWithFormatv(
          "Get value for register_name={0}, register_addr={1:x}, focus_core_id={2} failed",
          reg_info->name, addr, data_index);
      return error;
  }
  if (summary_info.reg_data.at(data_index).at(addr)->invalid) {
      error.SetErrorStringWithFormatv(
          "Get invalid value for register_name={0}, register_addr={1:x}, focus_core_id={2} failed",
          reg_info->name, addr, data_index);
    return error;
  }
  Log *log = GetLog(LLDBLog::Thread);
  const auto &core_reg_info = summary_info.reg_data.at(data_index).at(addr);
  if (core_reg_info->reg_size < reg_info->byte_size) {
    LLDB_LOG(
        log,
        "Warning: for reg {0}, core file byte size={1}<expect byte size={1}",
        reg_info->name, core_reg_info->reg_size, reg_info->byte_size);
  }
  value.SetFromMemoryData(*reg_info, core_reg_info->GetValue(),
                          reg_info->byte_size, lldb::eByteOrderLittle, error);
  // special fix execmask and simtpc
  if (std::string(reg_info->name) == "EXECMASK" && value.GetAsUInt32() == 0) {
    value.SetUInt32(UINT32_MAX);
  }
  return error;
}

bool RegisterContextPOSIXCore_ascend::ReadRegister(const RegisterInfo *reg_info,
                                                   RegisterValue &value) {
  Log *log = GetLog(LLDBLog::Thread);
  auto register_map = m_register_info->GetRegisterMap();
  m_summary_info = &GetThread().GetProcess()->GetSummaryInfo();

  if (reg_info && reg_info->kinds[lldb::eRegisterKindLLDB] < register_map.size()) {
    const auto *reg_name = reg_info->name;
    uint64_t addr;
    Status error = m_register_info->GetRegisterAddr(
        reg_name, m_summary_info->focus_pos_info.core_type,
        m_summary_info->focus_pos_info.pos_type, addr);
    if (error.Fail()) {
      LLDB_LOG(log, "Get addr for register_name={0} failed", reg_name);
      return false;
    }
    LLDB_LOG(log,
             "Start read register value for register_name={0}, pos_type={1}, "
             "core_type={2}, core_id={3}",
             reg_name,
             static_cast<uint32_t>(m_summary_info->focus_pos_info.pos_type),
             static_cast<uint32_t>(m_summary_info->focus_pos_info.core_type),
             m_summary_info->focus_pos_info.core_id);
    // added pos_info;
    InterruptPosInfo pos_info{};
    pos_info.core_type = m_summary_info->focus_pos_info.core_type;
    pos_info.core_id = m_summary_info->focus_pos_info.core_id -
                       GetMaxAicID(m_summary_info->chip_type);
    pos_info.pos_type = m_summary_info->focus_pos_info.pos_type;
    pos_info.thread_pos = m_summary_info->focus_pos_info.thread_pos;
    pos_info.thread_info = {m_summary_info->focus_pos_info.thread_dim.x,
                            m_summary_info->focus_pos_info.thread_dim.y,
                            m_summary_info->focus_pos_info.thread_dim.y};
    error = m_register_info->ReadRegister(reg_info, pos_info, this, value);
    if (error.Fail()) {
      LLDB_LOG(log, "Read register {0} failed: {1}", reg_name, error);
      return false;
    }
    // use error register to fix pc
    if (reg_info->kinds[lldb::eRegisterKindGeneric] == LLDB_REGNUM_GENERIC_PC) {
      uint64_t pc = value.GetAsUInt64();
      FixPC(pc);
      LLDB_LOG(log, "Update pc from {0:x} to {1:x}", value.GetAsUInt64(), pc);
      value.SetUInt64(pc);
    }
  } else {
    return false;
  }
  return true;
}

bool RegisterContextPOSIXCore_ascend::WriteRegister(const RegisterInfo *reg_info,
                                                   const RegisterValue &value) {
  return false;
}

void RegisterContextPOSIXCore_ascend::InvalidateAllRegisters() {}

size_t RegisterContextPOSIXCore_ascend::GetRegisterCount() {
  return m_register_info->GetRegisterCount();
}

const RegisterInfo *RegisterContextPOSIXCore_ascend::GetRegisterInfoAtIndex(size_t reg) {
  if (reg < GetRegisterCount())
    return &GetRegisterInfo()[reg];
  return nullptr;
}

size_t RegisterContextPOSIXCore_ascend::GetRegisterSetCount() {
  return m_register_info->GetRegisterSetCount();
}

const RegisterSet *RegisterContextPOSIXCore_ascend::GetRegisterSet(size_t set) {
  return m_register_info->GetRegisterSet(set);
}

const RegisterInfo *RegisterContextPOSIXCore_ascend::GetRegisterInfo() {
  return m_register_info->GetRegisterInfo();
}

#endif
