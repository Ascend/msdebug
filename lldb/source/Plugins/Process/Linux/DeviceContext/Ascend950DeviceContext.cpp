/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "Ascend950DeviceContext.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/RegisterValue.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend950.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/StreamString.h"
 
using namespace lldb_private;
using namespace lldb;
 
namespace debug_ts {
#pragma pack(4)
struct DeviceCoreMask {
  uint64_t aic_bitmap0;
  uint64_t aic_bitmap1;
  uint64_t aiv_bitmap0;
  uint64_t aiv_bitmap1;
};
 
struct CoreMaskParam {
  uint32_t magic {0U};
  DeviceCoreMask cores;
};
 
// single step 和 resume共用
struct ControlUnitParam {
  CoreMaskParam core_info;
  InterruptPosType pos_type;
  uint8_t enable_all_warp; // 使能所有warp
  uint16_t thread_id_x; // simt vf才需要用这三个确定使能哪个warp
  uint16_t thread_id_y;
  uint16_t thread_id_z;
};

struct InvalidCacheParam {
  uint8_t reserve0;
  uint8_t redirect_ifu;
  uint8_t reserve[2];
  uint64_t virt_addr;
  CoreMaskParam core_info;
};

struct HardBreakpointParam {
  uint8_t enable_all;
  uint8_t reserve;
  uint16_t stream_id;
  uint64_t virt_addr;
  DeviceCoreMask core_info; // 使能哪些core做断点设置
};
 
struct WarpInfoParam {
  uint8_t core_type;
  InterruptPosType pos_type;
  uint8_t core_id;
  uint8_t reserve0;
  uint16_t thread_id_x;
  uint16_t thread_id_y;
  uint16_t thread_id_z;
  uint16_t reserve1;
};
 
struct WarpInfo {
  uint8_t core_id;
  uint8_t warp_id;
  uint16_t warp_num; // 总共多少个warp需要获取, 其实没用，因为=thread_dim的相乘
  uint64_t simt_pc;
  uint32_t exec_mask; // 表示这个warp有效, >0表示有效，可以调试
};
#pragma pack()
 
} // namespace debug_ts
 
using namespace debug_ts;
 
Ascend950DeviceContext::Ascend950DeviceContext(const ::pid_t pid, const uint32_t device_id):
  DeviceContext(pid, device_id) {
  m_soc_type = SocType::ASCEND950;
}

Status Ascend950DeviceContext::ReadSXReg(const RegisterInfo *reg_info,
                                         const InterruptPosInfo &pos, RegisterValue &value) {
  Log *log = GetLog(LLDBLog::Process);

  auto reg_group = RegisterInfoPOSIX_ascend950::GetSRegGroup(reg_info);
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend950::GetRegExtractor().register_map;
  DataBufferHeap buffer;
  for (uint8_t i = 0; i < reg_group.num; i++) {
    const RegisterInfo *base_reg_info = RegisterInfoPOSIX_ascend950::GetRegisterInfoAt(i + reg_group.start_reg_num);
    RegisterValue tmp_value;
    if (base_reg_info && register_map.find(base_reg_info->name) != register_map.end()) {
      const DeviceRegisterInfo &device_reg_info = register_map.at(base_reg_info->name);
      error = DeviceContext::ReadRegister(device_reg_info.addr,
                                          base_reg_info, pos.core_id, pos.core_type, tmp_value);
      if (error.Fail()) {
        return error;
      }
      const uint8_t *value_data = static_cast<const uint8_t *>(tmp_value.GetBytes());
      uint8_t value_bytes = 4;
      if (reg_group.half_type == 1) {
        value_bytes = 2;
      } else if (reg_group.half_type == 2) {
        value_data += 2;
        value_bytes = 2;
      }
      // little order
      buffer.AppendData(value_data, value_bytes);
    }
  }
  value.SetBytes(buffer.GetBytes(), reg_info->byte_size, lldb::eByteOrderLittle);
  if (log) {
    StreamString ss;
    ss << "0x";
    uint8_t *cur_data = buffer.GetBytes();
    for (size_t i = 0; i < std::min(reg_info->byte_size, 8U); i++) {
      ss.Printf("%02x", cur_data[i]);
    }
    LLDB_LOG(log, "Read register {0} with start_reg={1}, group_num={2}, half_type={3}, "
             "first {4} byte value: {5}",
             reg_info->name, reg_group.start_reg_num,
             reg_group.num, reg_group.half_type,
             std::min(reg_info->byte_size, 8U), ss.GetString());
  }
  return error;
}

Status Ascend950DeviceContext::ReadRXReg(const RegisterInfo *reg_info, uint64_t base_addr,
                                         const InterruptPosInfo &pos, RegisterValue &value) {
  Log *log = GetLog(LLDBLog::Process);
  uint64_t rx_addr = base_addr;
  uint8_t r_idx = reg_info->kinds[eRegisterKindDWARF] - dwarf_r0_ascend;
  uint16_t thread_id = pos.thread_info.thread_id;
  uint16_t warp_id = pos.GetWarpId();
  rx_addr |= (warp_id % 4) << 10; // bit 11-10
  uint32_t wid_div = warp_id >> 2;
  rx_addr |= (wid_div & 0x1) << 9; // bit 9

  rx_addr |= (((wid_div >> 1) & 0x1) | ((r_idx >> 6) & 0x1)) << 8; // bit 8
  rx_addr |= (((wid_div >> 2) & 0x1) | ((r_idx >> 5) & 0x1)) << 7; // bit 7
  rx_addr |= (((wid_div >> 3) & 0x1) | ((r_idx >> 4) & 0x1)) << 6; // bit 6
  rx_addr |= (r_idx & 0xF) << 2; // bit 5-2
  rx_addr |= thread_id / 8; // maybe thread_idx > 32?
  RegisterInfo fix_byte_reg_info = *reg_info;
  fix_byte_reg_info.byte_size = 256;
  RegisterValue batch_thread_value;
  auto error = DeviceContext::ReadRegister(rx_addr, reg_info, pos.core_id, pos.core_type, batch_thread_value);
  if (error.Fail()) {
    return error;
  }
  const uint8_t *data = static_cast<const uint8_t*>(batch_thread_value.GetBytes());
  value.SetBytes(data + (thread_id % 8 * 32), 32, batch_thread_value.GetByteOrder());
  LLDB_LOG(log, "got R[{0}]={1:x}, warp_id={2}, thread_id={3}", r_idx, value.GetAsUInt64(), warp_id, thread_id);
  return error;
}

Status Ascend950DeviceContext::ReadSimtPC(const RegisterInfo *reg_info, uint64_t base_addr,
                                         const InterruptPosInfo &pos, RegisterValue &value) {
  Log *log = GetLog(LLDBLog::Process);
  uint16_t warp_id = pos.GetWarpId();
  Status error =  DeviceContext::ReadRegister(base_addr | warp_id, reg_info, pos.core_id, pos.core_type, value);
  LLDB_LOG(log, "got SimtPC={0:x}, warp_id={1}", value.GetAsUInt64(), warp_id);
  return error;
}
 
Status Ascend950DeviceContext::ReadRegister(const RegisterInfo *reg_info,
                                            const InterruptPosInfo &pos_info, RegisterValue &value) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend950::GetRegExtractor().register_map;
  if (reg_info && reg_info->kinds[eRegisterKindLLDB] < register_map.size() &&
      register_map.find(reg_info->name) != register_map.end()) {
    const DeviceRegisterInfo &device_reg_info = register_map.at(reg_info->name);
    uint64_t addr = device_reg_info.addr;

    // Read R0~R129
    if (reg_info->kinds[eRegisterKindDWARF] >= dwarf_r0_ascend && reg_info->kinds[eRegisterKindDWARF] <= dwarf_rx_max_id) {
      return ReadRXReg(reg_info, addr, pos_info, value);
    }
    if (RegisterInfoPOSIX_ascend950::IsSimtPC(reg_info)) {
      return ReadSimtPC(reg_info, addr, pos_info, value);
    }
    // Read S0~S60
    if (RegisterInfoPOSIX_ascend950::IsSReg(reg_info)) {
      return ReadSXReg(reg_info, pos_info, value);
    }

    error = DeviceContext::ReadRegister(addr, reg_info, pos_info.core_id, pos_info.core_type, value);
    if (error.Fail()) {
      return error;
    }
  } else {
    error.SetErrorString("reg_info is null or reg_num in reg_info is invalid");
  }
  return error;
}


Status Ascend950DeviceContext::GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend950::GetRegExtractor().register_map;
  auto reg_info = register_map.find(reg_name.str());
  if (reg_info == register_map.end()) {
      error.SetErrorStringWithFormatv(
          "Can not get addr, register name: {0}", reg_name);
  }
  if ((1U << static_cast<int>(core_type)) & reg_info->second.core_type_support_mask) {
    addr = reg_info->second.addr;
  } else {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register {0} is not support", reg_name);
  }
  return error;
}

Status Ascend950DeviceContext::GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) {
  Status error;
  if (core_type == CoreType::UNKNOWN_CORE_TYPE) {
    error.SetErrorString("GetRegisterList failed due to unknown core type.");
    return error;
  }
  const auto &register_map = RegisterInfoPOSIX_ascend950::GetRegExtractor().register_map;
  for(const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      reg_list.push_back(item.first);
    }
  }
  return error;
}

Status Ascend950DeviceContext::CheckRegisterAddr(CoreType core_type, uint64_t addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend950::GetRegExtractor().register_map;
  for (const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      if ((item.second.addr & addr) == item.second.addr) {
        return error;
      }
    }
  }
  error.SetErrorStringWithFormatv("CheckRegisterAddr failed due to error register addr, addr={0:x}, core_type={1}",
                                  addr, static_cast<uint8_t>(core_type));
  return error;
}

MemType Ascend950DeviceContext::GetStackMemType() const {
  return MemType::OUT_MEM;
}

Status Ascend950DeviceContext::GetDeviceInfo(DeviceInfo &device_info) {
  Status error;
  DeviceCoreMask info;
  Log *log = GetLog(LLDBLog::Process);
  error = BaseSqCqComm(CmdType::GET_DEVICES_INFO, nullptr, 0,
                       (uint8_t *)&info, sizeof(info));
  if (error.Fail()) {
    return error;
  }
  device_info.aic_bitmaps.clear();
  device_info.aiv_bitmaps.clear();
  device_info.aiv_bitmaps.push_back(info.aiv_bitmap0);
  device_info.aiv_bitmaps.push_back(info.aiv_bitmap1);
  device_info.aic_bitmaps.push_back(info.aic_bitmap0);
  device_info.aic_bitmaps.push_back(info.aic_bitmap1);
  device_info.device_id = m_device_id;
  LLDB_LOGF(log, "GetDeviceInfo device_id=%d aic_bitmap0=%#lx, aic_bitmap1=%#lx,"
            "aiv_bitmap0=%#lx, aiv_bitmap1=%#lx",
            device_info.device_id, info.aic_bitmap0, info.aic_bitmap1,
            info.aiv_bitmap0, info.aiv_bitmap1);
  return error;
}

inline CoreMaskParam GenCoreMask(const InterruptPosInfo &pos_info) {
  CoreMaskParam core_info{};
  if (!pos_info.single_core_run) {
    return core_info;
  }
  core_info.magic = 0x5a5a5a5a;
  auto &cores = core_info.cores;
  auto core_id = pos_info.core_id;
  constexpr uint8_t max_bit_num = 63;
  if (pos_info.core_type == CoreType::AIC) {
    if (core_id > max_bit_num) {
      cores.aic_bitmap1 = 1ULL << (core_id - max_bit_num);
    } else {
      cores.aic_bitmap0 = 1ULL << core_id;
    }
  } else if (pos_info.core_type == CoreType::AIV) {
    if (core_id > max_bit_num) {
      cores.aiv_bitmap1 = 1ULL << (core_id - max_bit_num);
    } else {
      cores.aiv_bitmap0 = 1ULL << core_id;
    }
  }
  return core_info;
}

inline auto FormatCoresLog(const DeviceCoreMask &cores) {
  return llvm::formatv("aic_bitmap0={0:x}, aic_bitmap1={1:x}, aiv_bitmap0={2:x}, aiv_bitmap1={3:x}",
          cores.aic_bitmap0, cores.aic_bitmap1, cores.aiv_bitmap0, cores.aiv_bitmap1);
}
 
Status Ascend950DeviceContext::Resume(const InterruptPosInfo &pos_info) const {
  ControlUnitParam param;
  param.core_info = GenCoreMask(pos_info);
  param.pos_type = pos_info.pos_type;
  if (pos_info.pos_type == InterruptPosType::STARS_VEC_INTERRUPT_SIMT) {
      if (pos_info.single_warp_run) {
          param.thread_id_x = pos_info.thread_pos.x;
          param.thread_id_y = pos_info.thread_pos.y;
          param.thread_id_z = pos_info.thread_pos.z;
      } else {
          param.enable_all_warp = true;
      }
  }
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "magic={0:x}, {1}, enable_all_warp={2}, thread_id_xyz=({3}, {4}, {5}), pos_type={6}",
           param.core_info.magic,
           FormatCoresLog(param.core_info.cores),
           param.enable_all_warp,
           param.thread_id_x, param.thread_id_y, param.thread_id_z,
           static_cast<uint8_t>(param.pos_type));
  return BaseSqCqComm(CmdType::RESUME_DEVICE, (uint8_t*)&param, sizeof(param));
}
 
Status Ascend950DeviceContext::SingleStep(const InterruptPosInfo &pos_info) const {
  ControlUnitParam param{};
  param.core_info = GenCoreMask(pos_info);
  if (pos_info.pos_type == InterruptPosType::STARS_VEC_INTERRUPT_SIMT) {
      if (pos_info.single_warp_run) {
          param.thread_id_x = pos_info.thread_pos.x;
          param.thread_id_y = pos_info.thread_pos.y;
          param.thread_id_z = pos_info.thread_pos.z;
      } else {
          param.enable_all_warp = true;
      }
  }
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "magic={0:x}, {1}, enable_all_warp={2}, thread_id_xyz=({3}, {4}, {5}), pos_type={6}",
           param.core_info.magic,
           FormatCoresLog(param.core_info.cores),
           param.enable_all_warp,
           param.thread_id_x, param.thread_id_y, param.thread_id_z,
           static_cast<uint8_t>(param.pos_type));
  return BaseSqCqComm(CmdType::SINGLE_STEP_DEVICE, (uint8_t*)&param, sizeof(param));
}

Status Ascend950DeviceContext::InvalidInstrCache(const lldb::addr_t &addr,
    const InterruptPosInfo &pos_info, uint8_t redirect_ifu) const {
  debug_ts::InvalidCacheParam param;
  param.reserve0 = 0;
  param.virt_addr = addr;
  param.redirect_ifu = redirect_ifu;
  param.core_info = GenCoreMask(pos_info);
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "magic={0:x}, {1}, virt_addr={2:x}, pos_type={3}",
           param.core_info.magic,
           FormatCoresLog(param.core_info.cores),
           param.virt_addr,
           static_cast<uint8_t>(pos_info.pos_type));
  return BaseSqCqComm(CmdType::INVALID_INSTR_CACHE, (uint8_t*)&param, sizeof(param));
}

Status Ascend950DeviceContext::SetHardwareBreakpoint(
    lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const {
  HardBreakpointParam param{};
  param.enable_all = 1;
  param.virt_addr = addr;
  param.stream_id = stream_id;
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "{0}, virt_addr={1:x}, stream_id={2}, pos_type={3}",
           FormatCoresLog(param.core_info),
           param.virt_addr,
           param.stream_id,
           static_cast<uint8_t>(pos_info.pos_type));
  return BaseSqCqComm(CmdType::SET_HARD_BREAKPOINT, (uint8_t*)&param, sizeof(param));
}
 
Status Ascend950DeviceContext::RemoveHardwareBreakpoint(
    lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const {
  HardBreakpointParam param;
  param.enable_all = 1;
  param.virt_addr = addr;
  param.stream_id = stream_id;
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "{0}, virt_addr={1:x}, stream_id={2}, pos_type={3}",
           FormatCoresLog(param.core_info),
           param.virt_addr,
           param.stream_id,
           static_cast<uint8_t>(pos_info.pos_type));
  return BaseSqCqComm(CmdType::UNSET_HARD_BREAKPOINT, (uint8_t*)&param, sizeof(param));
}
#endif // ifdef MS_DEBUGGER
