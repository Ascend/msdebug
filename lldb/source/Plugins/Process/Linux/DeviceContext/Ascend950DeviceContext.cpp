/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "Ascend950DeviceContext.h"
#include "lldb/Utility/LLDBLog.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend950.h"

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
  uint8_t bkpt_type;
  uint8_t core_id;
  uint8_t warp_id;
};

#pragma pack()

} // namespace debug_ts

using namespace debug_ts;

Ascend950DeviceContext::Ascend950DeviceContext(const ::pid_t pid, const uint32_t device_id):
  DeviceContext(pid, device_id) {
  m_soc_type = SocType::ASCEND950;
  m_reg_info_up = std::make_unique<RegisterInfoPOSIX_ascend950>(ArchSpec("hiipu64"));
}

// we should prevent user read custom addr in client
// or not let DeviceContext::ReadRegister() export to public
Status Ascend950DeviceContext::CheckRegisterAddr(CoreType core_type, uint64_t addr) const {
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

bool Ascend950DeviceContext::IsValidStack(addr_t addr, size_t size) {
  return true;
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
  // ignore aic_bitmap1 in this version
  device_info.aic_bitmaps.push_back(info.aic_bitmap0);
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
  if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
    if (pos_info.single_warp_run) {
      param.thread_id_x = pos_info.thread_pos.x;
      param.thread_id_y = pos_info.thread_pos.y;
      param.thread_id_z = pos_info.thread_pos.z;
    } else {
      param.enable_all_warp = 1;
    }
  }
  param.pos_type = pos_info.pos_type;
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
  if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
    if (pos_info.single_warp_run) {
      param.thread_id_x = pos_info.thread_pos.x;
      param.thread_id_y = pos_info.thread_pos.y;
      param.thread_id_z = pos_info.thread_pos.z;
    } else {
      param.enable_all_warp = 1;
    }
  }
  param.pos_type = pos_info.pos_type;
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

Status
Ascend950DeviceContext::GetWarpsInfo(std::vector<WarpInfo> &warps_info,
                                     const InterruptPosInfo &m_pos_info) const {
  warps_info.clear();
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  WarpInfo warp_info;
  uint16_t warp_id = 0;
  error = GetWarpInfo(warp_info, warp_id, m_pos_info);
  if (error.Fail()) {
    return error;
  }

  // 当接口没有返回warp num的时候，需要自己计算下
  uint16_t warp_num = 0;
  if (warp_info.warp_num == 0) {
    warp_num = m_pos_info.GetWarpNum();
    warp_info.warp_num = warp_num;
  }

  LLDB_LOGF(log, "GetWarpsInfo total_num=%u", warp_info.warp_num);

  warps_info.push_back(warp_info);

  for (uint16_t i = 1; i < warp_info.warp_num; i++) {
    warp_id = i;
    error = GetWarpInfo(warp_info, warp_id, m_pos_info);
    if (error.Fail()) {
      error.SetErrorStringWithFormatv("get {0} warp failed: {1}", i , error);
      return error;
    }
    if (warp_info.warp_num == 0) {
      warp_info.warp_num = warp_num;
    }

    if (warp_info.warp_id != warp_id) {
      error.SetErrorStringWithFormatv("get {0} warp failed: not matched warp id", i);
      return error;
    }
    LLDB_LOG(log, "warp_id={0}, warp_pc={1:x}", warp_info.warp_id, warp_info.simt_pc);
    warps_info.push_back(warp_info);
  }
  return error;
}

Status Ascend950DeviceContext::GetWarpInfo(WarpInfo &info,  uint16_t warp_id,
                                           const InterruptPosInfo &m_pos_info) const {
  WarpInfoParam param{};
  param.core_type = static_cast<uint8_t>(m_pos_info.core_type);
  param.core_id = m_pos_info.core_id;
  param.bkpt_type = 2;
  param.warp_id = warp_id;

  return BaseSqCqComm(CmdType::GET_WARP_INFO, (uint8_t*)&param, sizeof(WarpInfoParam),
                        (uint8_t *)&info, sizeof(WarpInfo));
}
#endif // ifdef MS_DEBUGGER
