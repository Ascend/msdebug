/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef liblldb_DEVICE_CONTEXT_H_
#define liblldb_DEVICE_CONTEXT_H_

#include "lldb/Utility/Status.h"
#include "lldb/Utility/MessageDefines.h"
#include "lldb/Host/ThreadLauncher.h"
#include "lldb/lldb-private-types.h"
#include <condition_variable>
#include <lldb/Host/Socket.h>
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend.h"
#include <mutex>


namespace lldb_private {

// ===== need to stay in namespace ts START ====
enum class CmdType {
    ENABLE_DEBUG_MODE = 0,
    DISABLE_DEBUG_MODE = 1,

    SET_HARD_BREAKPOINT = 2,
    UNSET_HARD_BREAKPOINT = 3,

    INVALID_INSTR_CACHE = 4,
    SINGLE_STEP_DEVICE = 5,

    RESUME_DEVICE = 6,
    SUSPEND_DEVICE = 7,

    GET_DEVICES_INFO = 8,
    GET_CORES_INFO = 9,

    READ_REGISTER = 10,
    READ_LOCAL_MEMORY = 11,
    GET_WARP_INFO = 12,
    TASK_KILL = 14,

    // -------------- Receive ------------
    INTERRUPT_EVENT = 101,
};

struct DebugRecvInfo {
  uint8_t phase;
  uint8_t reserved[3];
  uint16_t sq_index;
  uint16_t sq_head;
  CmdType cmd_type;
  uint32_t return_val;
  uint32_t msg_id;
  uint8_t recv_msg[44];
};

struct TsDeviceInfo {
  uint64_t aic_bitmap;
  uint64_t aiv_bitmap;
};

struct CoreInfoParam {
  uint16_t info_idx;
  uint8_t require_err_info; // 1 表示本次只需要err信息
  uint8_t reserve;
};

// require_err_info = 1时用这个结构体接收返回值
struct CoreErrorInfo {
  uint64_t aix_error_bitmap0; // CUBE_ERROR only aic
  uint32_t aix_error_bitmap1; // SU_ERROR
  uint32_t aix_error_bitmap2; // SC_ERROR
  uint64_t aix_error_bitmap3; // VEC_ERROR only aiv
  uint32_t aix_error_bitmap4; // MTE_ERROR
  uint64_t aix_error_bitmap5; // L1_ERROR only aic
};

struct CoreMaskInfo {
  uint32_t magic {0U};
  uint32_t aic_mask {0U};
  uint64_t aiv_mask {0U};
};

struct InvalidCacheParam {
  uint8_t enable_all;
  uint8_t redirect_ifu;
  uint8_t reserve[2];
  uint64_t virt_addr;
  CoreMaskInfo core_info;
};

// ============ ts END ==================

class DeviceContext : public RegisterDataInterface {
public:

class Factory {
public:
  static std::shared_ptr<DeviceContext> GetDeviceContext(const std::string &soc_version, ::pid_t pid, int device_id);
};

  typedef std::function<void(const DebugRecvInfo &info)> Callback;
  DeviceContext(const ::pid_t pid, const uint32_t device_id);
  ~DeviceContext();
  virtual Status Init();

  virtual Status Resume(const InterruptPosInfo &pos_info) const;
  virtual size_t ReadMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                            const InterruptPosInfo &pos_info, void *out);

  virtual Status SingleStep(const InterruptPosInfo &pos_info) const;

  virtual Status SetSoftwareBreakpoint(lldb::addr_t addr);
  virtual Status SetHardwareBreakpoint(lldb::addr_t addr, uint16_t stream_id,
                                       const InterruptPosInfo &pos_info) const {
      return Status("Unsupport hardware breakpoint.");
  }
  virtual Status RemoveSoftwareBreakpoint(lldb::addr_t addr);
  virtual Status RemoveHardwareBreakpoint(lldb::addr_t addr, uint16_t stream_id,
                                          const InterruptPosInfo &pos_info) const {
    return Status("Unsupport hardware breakpoint.");
  }

  // used by AscendProcessLinux
  Status ReadRegister(const RegisterInfo *reg_info, const InterruptPosInfo &pos_info, RegisterValue &value) const;

  // implement RegisterDataInterface, just read data from driver
  Status ReadRegister(uint64_t addr, const RegisterInfo *reg_info,
                      uint32_t core_id, CoreType core_type, RegisterValue &value) const override;

  virtual Status GetDeviceInfo(DeviceInfo &device_info);
  virtual Status GetCoresInfo(std::vector<CoreInfo> &cores_info);
  virtual Status GetWarpsInfo(std::vector<WarpInfo> &warps_info, const InterruptPosInfo &m_pos_info) const {
    return Status("Unsupport query warps info.");
  }
  virtual Status GetWarpInfo(WarpInfo &info, uint16_t warp_id, const InterruptPosInfo &m_pos_info) const {
    return Status("Unsupport query warp info.");
  }
  // some register may be unavaliable in aiv or aic, so we need do a check
  virtual Status CheckRegisterAddr(CoreType core_type, uint64_t addr) const = 0;
  virtual void SetBreakpointCallback(const Callback &callback);

  virtual bool StartListenThread();
  virtual Status EnableDebugMode();
  virtual Status InvalidInstrCache(const lldb::addr_t &addr,
                                   const InterruptPosInfo &pos_info,
                                   uint8_t redirect_ifu = 0) const;
  virtual size_t ReadLocalMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                 const InterruptPosInfo &pos_info, void *data);
  virtual size_t ReadGlobalMemory(lldb::addr_t addr, size_t size, void *data);
  virtual size_t WriteGlobalMemory(lldb::addr_t addr, size_t size, const void *data);
  virtual bool SupportDirectlySetSwBp() { return false; }
  virtual SocType GetSocType() = 0;
  void SetCanRun(bool can);
  virtual bool UseSpecifiedCore(uint32_t aic_mask, uint64_t aiv_mask, CoreMaskInfo &maskInfo);
  virtual Status TaskKill(uint32_t stream_id);
  virtual MemType DeviceAddressClassToMemType(const DeviceAddressClass address_class) const;
  virtual bool IsValidStack(lldb::addr_t addr, size_t size);
  virtual MemType GetStackMemType() const = 0;
  Status BaseSqCqComm(CmdType type, const uint8_t *data = nullptr, const uint8_t len = 0,
                      uint8_t *out = nullptr, uint8_t out_len = 0) const;
  void SetSocket(const Socket *client_socket);
  bool IsDeviceIdMatched(const uint32_t device_id) const;
  bool IsTgidMatched(const int32_t tgid) const;
  void UpdateTgid(const int32_t tgid);
  virtual bool IsValidGlobalAddr(lldb::addr_t addr, size_t size);
  virtual void AddIpcMemInfo(lldb::addr_t addr, size_t size,
                             std::vector<char> key) {
    return;
  }
  virtual void RemoveIpcMemInfo(lldb::addr_t addr) { return; }

protected:
  int32_t m_drv_fd;
  int32_t m_pid;
  uint32_t m_device_id;
  Callback m_bp_callback;
  bool m_close;
  HostThread m_listen_thread;
  std::vector<CoreInfo> m_cores_info;
  uint32_t m_num_cores;
  std::mutex m_mtx;
  std::condition_variable m_cv;
  Status m_init_err;
  const Socket *m_client_socket = nullptr;
  std::unique_ptr<RegisterInfoPOSIX_ascend> m_reg_info_up;

private:
  lldb::thread_result_t RunListenThread();
};

} // namespace lldb_private

#endif // #ifndef liblldb_DEVICE_CONTEXT_
#endif // #ifdef MS_DEBUGGER
