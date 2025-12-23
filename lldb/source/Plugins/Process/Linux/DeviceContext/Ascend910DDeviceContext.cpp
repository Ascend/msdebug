/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "Ascend910DDeviceContext.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/RegisterValue.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend910D.h"
 
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
  uint8_t reserve[2];
  uint16_t stream_id;
  uint64_t virt_addr;
  CoreMaskParam core_info; // 使能哪些core做断点设置
};
 
struct WarpInfoParam {
  uint16_t core_type;
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
 
Ascend910DDeviceContext::Ascend910DDeviceContext(const ::pid_t pid, const uint32_t device_id):
  DeviceContext(pid, device_id) {
  m_soc_type = SocType::ASCEND910D;
}
 
Status Ascend910DDeviceContext::ReadRegister(const RegisterInfo *reg_info,
                                             uint32_t core_id, CoreType core_type, RegisterValue &value) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910D::GetRegExtractor().register_map;
  if (reg_info && reg_info->kinds[eRegisterKindLLDB] < register_map.size() &&
      register_map.find(reg_info->name) != register_map.end()) {
    const DeviceRegisterInfo &device_reg_info = register_map.at(reg_info->name);
    uint64_t addr = device_reg_info.addr;
    uint64_t value_int;
    error = DeviceContext::ReadRegister(addr, core_id, core_type, value_int);
    if (error.Fail()) {
      return error;
    }
    value.SetUInt64(value_int);
  } else {
    error.SetErrorString("reg_info is null or reg_num in reg_info is invalid");
  }
  return error;
}

Status Ascend910DDeviceContext::ReadRegister(uint64_t addr, uint32_t core_id, CoreType core_type, uint64_t &value) {
  addr = RegisterInfoPOSIX_ascend910D::GetDbgAddr(addr);
  return DeviceContext::ReadRegister(addr, core_id, core_type, value);
}

Status Ascend910DDeviceContext::GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910D::GetRegExtractor().register_map;
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

Status Ascend910DDeviceContext::GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) {
  Status error;
  if (core_type == CoreType::UNKNOWN_CORE_TYPE) {
    error.SetErrorString("GetRegisterList failed due to unknown core type.");
    return error;
  }
  const auto &register_map = RegisterInfoPOSIX_ascend910D::GetRegExtractor().register_map;
  for(const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      reg_list.push_back(item.first);
    }
  }
  return error;
}

Status Ascend910DDeviceContext::CheckRegisterAddr(CoreType core_type, uint64_t addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910D::GetRegExtractor().register_map;
  for (const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      if (item.second.addr == addr) {
        return error;
      }
    }
  }
  error.SetErrorStringWithFormatv("CheckRegisterAddr failed due to error register addr, addr={0:x}, core_type={1}",
                                  addr, static_cast<uint8_t>(core_type));
  return error;
}

MemType Ascend910DDeviceContext::GetStackMemType() const {
  return MemType::OUT_MEM;
}

Status Ascend910DDeviceContext::GetDeviceInfo(DeviceInfo &device_info) {
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
 
Status Ascend910DDeviceContext::Resume(const InterruptPosInfo &pos_info) const {
  ControlUnitParam param;
  param.core_info = GenCoreMask(pos_info);
  if (pos_info.pos_type == InterruptPosType::STARS_VEC_INTERRUPT_SIMT) {
      if (pos_info.single_warp_run) {
          param.thread_id_x = pos_info.thread_id_x;
          param.thread_id_y = pos_info.thread_id_y;
          param.thread_id_z = pos_info.thread_id_z;
      } else {
          param.enable_all_warp = true;
      }
  }
  return BaseSqCqComm(CmdType::RESUME_DEVICE, (uint8_t*)&param, sizeof(param));
}
 
Status Ascend910DDeviceContext::SingleStep(const InterruptPosInfo &pos_info) const {
  ControlUnitParam param{};
  param.core_info = GenCoreMask(pos_info);
  if (pos_info.pos_type == InterruptPosType::STARS_VEC_INTERRUPT_SIMT) {
      if (pos_info.single_warp_run) {
          param.thread_id_x = pos_info.thread_id_x;
          param.thread_id_y = pos_info.thread_id_y;
          param.thread_id_z = pos_info.thread_id_z;
      } else {
          param.enable_all_warp = true;
      }
  }
  return BaseSqCqComm(CmdType::SINGLE_STEP_DEVICE, (uint8_t*)&param, sizeof(param));
}

Status Ascend910DDeviceContext::InvalidInstrCache(const lldb::addr_t &addr,
    const InterruptPosInfo &pos_info, uint8_t redirect_ifu) const {
  debug_ts::InvalidCacheParam param;
  param.reserve0 = 0;
  param.virt_addr = addr;
  param.redirect_ifu = redirect_ifu;
  param.core_info = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::INVALID_INSTR_CACHE, (uint8_t*)&param, sizeof(param));
}

Status Ascend910DDeviceContext::SetHardwareBreakpoint(
    lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const {
  HardBreakpointParam param;
  param.virt_addr = addr;
  param.stream_id = stream_id;
  param.core_info = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::SET_HARD_BREAKPOINT, (uint8_t*)&param, sizeof(param));
}
 
Status Ascend910DDeviceContext::RemoveHardwareBreakpoint(
    lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const {
  HardBreakpointParam param;
  param.virt_addr = addr;
  param.stream_id = stream_id;
  param.core_info = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::UNSET_HARD_BREAKPOINT, (uint8_t*)&param, sizeof(param));
}
#endif // ifdef MS_DEBUGGER