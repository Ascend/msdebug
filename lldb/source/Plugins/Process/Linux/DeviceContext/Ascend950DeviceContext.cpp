/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "Ascend950DeviceContext.h"

#include <dlfcn.h>

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

class IpcMemoryMapper {
  struct MemoryMapping {
    uint64_t phys_start;
    uint64_t virt_start;
    size_t size;
    std::vector<char> key;

    bool operator<(const MemoryMapping &other) const {
      return phys_start < other.phys_start;
    }
  };

public:
  bool AddMapping(uint64_t phys_addr, uint64_t virt_addr, size_t size,
                  const std::vector<char> &key) {
    MemoryMapping new_map{phys_addr, virt_addr, size, key};

    // 检查重叠
    auto it = m_mappings.lower_bound(new_map);

    // 与前一个区间比较
    if (it != m_mappings.begin()) {
      auto prev = std::prev(it);
      if (prev->phys_start + prev->size > phys_addr) {
        return false;
      }
    }

    // 与后一个区间比较
    if (it != m_mappings.end()) {
      if (phys_addr + size > it->phys_start) {
        return false;
      }
    }

    m_mappings.insert(new_map);
    return true;
  }

  bool Translate(uint64_t phys_addr, uint64_t &virt_addr,
                 uint64_t &offset) const {
    MemoryMapping key{phys_addr, 0, 0, {}};
    auto it = m_mappings.lower_bound(key);
    if (it != m_mappings.begin()) {
      --it;
      const auto &m = *it;
      uint64_t phys_end = m.phys_start + m.size;

      if (phys_addr >= m.phys_start && phys_addr < phys_end) {
        offset = phys_addr - m.phys_start;
        virt_addr = m.virt_start + offset;
        return true;
      }
    }
    return false;
  }

  // 移除映射
  void RemoveMapping(uint64_t addr) {
    MemoryMapping key{addr, 0, 0, {}};
    auto it = m_mappings.find(key);
    if (it != m_mappings.end()) {
      m_mappings.erase(it);
    }
  }

  bool QuaryKey(uint64_t addr, std::vector<char> &key) {
    MemoryMapping tmp{addr, 0, 0, {}};
    auto it = m_mappings.find(tmp);
    if (it != m_mappings.end()) {
      key = it->key;
      return true;
    }
    return false;
  }

  bool IsFullyMapped(uint64_t phys_addr, size_t size) const {
    if (size == 0) {
      return true; // 空区间视为合法
    }

    uint64_t req_end = phys_addr + size;

    // 找到第一个 phys_start >= phys_addr 的映射
    MemoryMapping key{phys_addr, 0, 0};
    auto it = m_mappings.lower_bound(key);

    // 如果第一个映射起点大于 phys_addr，说明前面有空洞
    if (it != m_mappings.begin()) {
      --it;
    }

    uint64_t covered_until = phys_addr;

    while (covered_until < req_end) {
      // 当前没有任何映射
      if (it == m_mappings.end()) {
        return false;
      }

      const auto &m = *it;

      // 当前映射起点已经超过已覆盖位置, 出现空洞
      if (m.phys_start > covered_until) {
        return false;
      }

      uint64_t m_end = m.phys_start + m.size;

      // 映射结束位置在当前所需之前, 不足
      if (m_end <= covered_until) {
        ++it;
        continue;
      }

      // 扩展已覆盖区域
      covered_until = m_end;
      ++it;
    }

    return true;
  }

private:
  std::set<MemoryMapping> m_mappings;
};

static IpcMemoryMapper g_ipc_mapper;

#define ACL_MEMCPY_HOST_TO_HOST 0
#define ACL_MEMCPY_HOST_TO_DEVICE 1
#define ACL_MEMCPY_DEVICE_TO_HOST 2
#define ACL_RT_IPC_MEM_IMPORT_FLAG_DEFAULT 0x0UL
#define ACL_RT_IPC_MEM_IMPORT_FLAG_ENABLE_PEER_ACCESS 0x1UL

class AclWrapper {
  using aclInitFuncType = int (*)(const char *);
  using aclrtGetDeviceFuncType = int (*)(int *);
  using aclrtSetDeviceFuncType = int (*)(int);
  using aclrtMemcpyFuncType = int (*)(void *dst, size_t destMax,
                                      const void *src, size_t count, int kind);
  using aclrtIpcMemImportByKeyFuncType = int (*)(void **devPtr, const char *key,
                                                 uint64_t flags);
  using aclrtIpcMemCloseFuncType = int (*)(const char *key);

public:
  ~AclWrapper();
  Status Init();
  int32_t AclInitWrapper();
  int32_t AclrtGetDeviceWrapper(int32_t *device_id);
  int32_t AclrtSetDeviceWrapper(int32_t device_id);
  int32_t AclrtMemcpyWrapper(void *dst, void *src, size_t size,
                             int32_t direction);
  int32_t AclrtIpcMemImportByKeyWrapper(void **sharedAddr,
                                        std::vector<char> key);
  int32_t AclrtIpcMemCloseWrapper(std::vector<char> key);

private:
  void *m_aclrt_handle{nullptr};
  void *m_aclInitFunc{nullptr};
  void *m_aclrtGetDeviceFunc{nullptr};
  void *m_aclrtSetDeviceFunc{nullptr};
  void *m_aclrtMemcpyFunc{nullptr};
  void *m_aclrtIpcMemImportByKeyFunc{nullptr};
  void *m_aclrtIpcMemCloseFunc{nullptr};
};

static AclWrapper g_aclWrapper;

AclWrapper::~AclWrapper() {
  if (m_aclrt_handle != nullptr) {
    dlclose(m_aclrt_handle);
  }
}

Status AclWrapper::Init() {
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  int ret = 0;

  if (m_aclrt_handle == nullptr) {
    m_aclrt_handle = dlopen("libacl_rt.so", RTLD_NOW | RTLD_GLOBAL);
    if (m_aclrt_handle == nullptr) {
      error.SetErrorStringWithFormat("dlopen libacl_rt.so failed");
      return error;
    }
  }
  m_aclInitFunc = dlsym(m_aclrt_handle, "aclInit");
  if (m_aclInitFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclInit failed");
    return error;
  }
  m_aclrtGetDeviceFunc = dlsym(m_aclrt_handle, "aclrtGetDevice");
  if (m_aclrtGetDeviceFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclrtGetDevice failed");
    return error;
  }
  m_aclrtSetDeviceFunc = dlsym(m_aclrt_handle, "aclrtSetDevice");
  if (m_aclrtSetDeviceFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclrtSetDevice failed");
    return error;
  }
  m_aclrtMemcpyFunc = dlsym(m_aclrt_handle, "aclrtMemcpy");
  if (m_aclrtMemcpyFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclrtMemcpy failed");
    return error;
  }
  m_aclrtIpcMemImportByKeyFunc =
      dlsym(m_aclrt_handle, "aclrtIpcMemImportByKey");
  if (m_aclrtIpcMemImportByKeyFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclrtIpcMemImportByKey failed");
    return error;
  }
  m_aclrtIpcMemCloseFunc = dlsym(m_aclrt_handle, "aclrtIpcMemClose");
  if (m_aclrtIpcMemCloseFunc == nullptr) {
    error.SetErrorStringWithFormat("dlsym aclrtIpcMemClose failed");
    return error;
  }

  return error;
}

int32_t AclWrapper::AclInitWrapper() {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclInitFunc == nullptr) {
    LLDB_LOG(log, "m_aclInitFunc is null");
    return -1;
  }
  int32_t ret = ((aclInitFuncType)m_aclInitFunc)(nullptr);
  if (ret != 0) {
    LLDB_LOGF(log, "aclInit failed ret=%d", ret);
    return ret;
  }
  LLDB_LOG(log, "aclInit done");
  return ret;
}

int32_t AclWrapper::AclrtGetDeviceWrapper(int32_t *device_id) {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclrtGetDeviceFunc == nullptr) {
    LLDB_LOG(log, "m_aclrtGetDeviceFunc is null");
    return -1;
  }
  int32_t ret = ((aclrtGetDeviceFuncType)m_aclrtGetDeviceFunc)(device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "aclrtGetDevice failed ret=%d", ret);
    return ret;
  }
  return ret;
}

int32_t AclWrapper::AclrtSetDeviceWrapper(int32_t device_id) {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclrtSetDeviceFunc == nullptr) {
    LLDB_LOG(log, "m_aclrtSetDeviceFunc is null");
    return -1;
  }
  int32_t ret = ((aclrtSetDeviceFuncType)m_aclrtSetDeviceFunc)(device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "aclrtSetDevice failed ret=%d device_id=%d", ret, device_id);
    return ret;
  }
  return ret;
}

int32_t AclWrapper::AclrtMemcpyWrapper(void *dst, void *src, size_t size,
                                       int32_t direction) {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclrtMemcpyFunc == nullptr) {
    LLDB_LOG(log, "m_aclrtMemcpyFunc is null");
    return -1;
  }

  int32_t ret =
      ((aclrtMemcpyFuncType)m_aclrtMemcpyFunc)(dst, size, src, size, direction);
  if (ret != 0) {
    LLDB_LOGF(log, "aclrtMemcpy failed ret=%d", ret);
    return ret;
  }
  return 0;
}

int32_t AclWrapper::AclrtIpcMemImportByKeyWrapper(void **sharedAddr,
                                                  std::vector<char> key) {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclrtIpcMemImportByKeyFunc == nullptr) {
    LLDB_LOG(log, "m_aclrtIpcMemImportByKeyFunc is null");
    return -1;
  }

  auto ret = ((aclrtIpcMemImportByKeyFuncType)m_aclrtIpcMemImportByKeyFunc)(
      sharedAddr, key.data(), ACL_RT_IPC_MEM_IMPORT_FLAG_ENABLE_PEER_ACCESS);
  if (ret != 0) {
    LLDB_LOG(log, "aclrtIpcMemImportByKey failed ret={0}", ret);
    return -1;
  }

  LLDB_LOG(log, "aclrtIpcMemImportByKey done ret={0}", ret);
  return 0;
}

int32_t AclWrapper::AclrtIpcMemCloseWrapper(std::vector<char> key) {
  Log *log = GetLog(LLDBLog::Process);
  if (m_aclrt_handle == nullptr) {
    LLDB_LOG(log, "m_aclrt_handle is null");
    return -1;
  }
  if (m_aclrtIpcMemCloseFunc == nullptr) {
    LLDB_LOG(log, "m_aclrtIpcMemCloseFunc is null");
    return -1;
  }

  auto ret = ((aclrtIpcMemCloseFuncType)m_aclrtIpcMemCloseFunc)(key.data());
  if (ret != 0) {
    LLDB_LOG(log, "aclrtIpcMemClose failed ret={0}", ret);
    return -1;
  }

  LLDB_LOG(log, "aclrtIpcMemClose done ret={0}", ret);
  return 0;
}

int32_t CheckDevice(int32_t target_device_id) {
  Log *log = GetLog(LLDBLog::Process);
  int32_t ret;
  int32_t device_id = -1;
  ret = g_aclWrapper.AclrtGetDeviceWrapper(&device_id);
  if (ret != 0 || (device_id != target_device_id)) {
    LLDB_LOGF(
        log, "aclrtGetDevice failed ret=%d device_id=%d try setDevice %d again",
        ret, device_id, target_device_id);
    ret = g_aclWrapper.AclrtSetDeviceWrapper(target_device_id);
    LLDB_LOGF(log, "aclrtSetDevice ret=%d device_id=%d", ret, target_device_id);
  }
  return ret;
}

Status Ascend950DTDeviceContext::Init() {
  Log *log = GetLog(LLDBLog::Process);
  Status error = DeviceContext::Init();
  if (error.Fail()) {
    LLDB_LOG(log, "DeviceContext Init failed: {0}", error);
    return error;
  }
  error = g_aclWrapper.Init();
  if (error.Fail()) {
    LLDB_LOG(log, "AclWrapper Init failed: {0}", error);
    return error;
  }

  int32_t ret;
  ret = g_aclWrapper.AclInitWrapper();
  if (ret != 0) {
    error.SetErrorStringWithFormat("aclInit failed ret=%d", ret);
    return error;
  }
  LLDB_LOG(log, "aclInit done");
  ret = g_aclWrapper.AclrtSetDeviceWrapper(m_device_id);
  if (ret != 0) {
    error.SetErrorStringWithFormat("aclrtSetDevice failed ret=%d device_id=%d",
                                   ret, m_device_id);
    return error;
  }
  LLDB_LOG(log, "aclrtSetDevice done device_id=%d", m_device_id);

  return error;
}

size_t Ascend950DTDeviceContext::ReadGlobalMemory(addr_t addr, size_t size,
                                                  void *data) {
  Log *log = GetLog(LLDBLog::Process);
  if (size == 0 || m_drv_fd == -1) {
    LLDB_LOG(
        log,
        "DeviceContext has not be initialized success or size=0, reason {0}",
        m_init_err);
    return 0;
  }
  if (!IsValidGlobalAddr(addr, size)) {
    LLDB_LOG(log, "Invalid global addr={0}, size={1}", addr, size);
    return 0;
  }

  if (!g_ipc_mapper.IsFullyMapped(addr, size)) {
    LLDB_LOGF(log, "mem not fully mapped on ipc mem. addr=0x%lx size=%lu", addr,
              size);
    return 0;
  }

  uint64_t sharedAddr = 0UL;
  uint64_t offset = 0UL;
  if (!g_ipc_mapper.Translate(addr, sharedAddr, offset)) {
    LLDB_LOGF(log, "ipc mem translate failed addr=0x%lx", addr);
    return 0;
  }

  int ret = CheckDevice(m_device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "CheckDevice failed ret=%d", ret);
    return 0;
  }
  ret = g_aclWrapper.AclrtMemcpyWrapper(data, (void *)sharedAddr, size,
                                        ACL_MEMCPY_DEVICE_TO_HOST);
  if (ret != 0) {
    LLDB_LOGF(log, "ReadGlobalMemory failed ret=%d", ret);
    return 0;
  }
  return size;
}

size_t Ascend950DTDeviceContext::WriteGlobalMemory(addr_t addr, size_t size,
                                                   const void *data) {
  Log *log = GetLog(LLDBLog::Process);
  if (size == 0 || m_drv_fd == -1) {
    LLDB_LOG(
        log,
        "DeviceContext has not be initialized success or size=0, reason {0}",
        m_init_err);
  }
  if (!IsValidGlobalAddr(addr, size)) {
    LLDB_LOG(log, "Invalid global addr={0}, size={1}", addr, size);
    return 0;
  }

  if (!g_ipc_mapper.IsFullyMapped(addr, size)) {
    LLDB_LOGF(log, "mem not fully mapped on ipc mem. addr=0x%lx size=%lu", addr,
              size);
    return 0;
  }

  uint64_t sharedAddr = 0UL;
  uint64_t offset = 0UL;
  if (!g_ipc_mapper.Translate(addr, sharedAddr, offset)) {
    LLDB_LOGF(log, "ipc mem translate failed addr=0x%lx", addr);
    return 0;
  }

  int ret = CheckDevice(m_device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "CheckDevice failed ret=%d", ret);
    return 0;
  }
  ret = g_aclWrapper.AclrtMemcpyWrapper((void *)sharedAddr, (void *)data, size,
                                        ACL_MEMCPY_HOST_TO_DEVICE);
  if (ret != 0) {
    LLDB_LOGF(log, "ReadGlobalMemory failed ret=%d", ret);
    return 0;
  }
  return size;
}

void Ascend950DTDeviceContext::AddIpcMemInfo(lldb::addr_t addr, size_t size,
                                             std::vector<char> key) {
  Log *log = GetLog(LLDBLog::Process);
  int ret = CheckDevice(m_device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "CheckDevice failed ret=%d", ret);
    return;
  }
  void *sharedAddr = nullptr;
  ret = g_aclWrapper.AclrtIpcMemImportByKeyWrapper(&sharedAddr, key);
  if ((ret != 0) || (sharedAddr == nullptr)) {
    return;
  }

  if (!g_ipc_mapper.AddMapping(addr, (uint64_t)sharedAddr, size, key)) {
    LLDB_LOG(log, "AddMapping failed");
    return;
  }
  LLDB_LOG(log, "AddIpcMemInfo done addr=0x%lx sharedAddr=0x%lx size=%lu", addr,
           sharedAddr, size);
}

void Ascend950DTDeviceContext::RemoveIpcMemInfo(lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Process);
  int ret = CheckDevice(m_device_id);
  if (ret != 0) {
    LLDB_LOGF(log, "CheckDevice failed ret=%d", ret);
    return;
  }

  // quary key
  std::vector<char> key;
  if (!g_ipc_mapper.QuaryKey(addr, key)) {
    LLDB_LOGF(log, "addr 0x%lx is not in ipc map", addr);
    return;
  }

  // close ipc mem
  ret = g_aclWrapper.AclrtIpcMemCloseWrapper(key);
  if (ret != 0) {
    LLDB_LOGF(log, "ipc mem close failed");
    return;
  }

  // remove addr from map
  g_ipc_mapper.RemoveMapping(addr);

  LLDB_LOG(log, "RemoveIpcMemInfo done addr=0x%lx ", addr);
}

#endif // ifdef MS_DEBUGGER
