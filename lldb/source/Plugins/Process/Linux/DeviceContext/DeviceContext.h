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
#include <mutex>


namespace lldb_private {

// ===== need to stay in namespace ts START ====
enum class CoreStatus {
  UNKNOWN = 0,
  RUNNING = 1,
  BRKPT = 2,
  SINGLE_STEP = 3,
  EXCEPTION = 4,
  TASK_KILLED = 5
};

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

struct InterruptEvent {
  uint8_t core_type;
  InterruptPosType pos_type;
  uint8_t reserve[2];
  CoreStatus status;
  uint32_t core_id;
  uint64_t pc;
  uint16_t thread_dim_x; // pos_type == 2才有效
  uint16_t thread_dim_y;
  uint16_t thread_dim_z;
  uint16_t thread_x; // pos_type == 2才有效
  uint16_t thread_y;
  uint16_t thread_z;
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
 
struct InterruptPosInfo {
  CoreType core_type;
  bool single_core_run;
  bool single_warp_run;
  uint32_t core_id;
  InterruptPosType pos_type{InterruptPosType::STARS_SU_INTERRUPT};
  uint16_t thread_id_x;
  uint16_t thread_id_y;
  uint16_t thread_id_z;
};
// ============ ts END ==================
 
class DeviceContext {
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

  virtual Status ReadRegister(const RegisterInfo *reg_info,
                              uint32_t core_id, CoreType core_type, RegisterValue &value) = 0;
  virtual Status ReadRegister(uint64_t addr, uint32_t core_id, CoreType core_type, uint64_t &value);
  virtual Status ReadRegister(const llvm::StringRef reg_name, uint32_t core_id, CoreType core_type, uint64_t &value);
  virtual Status ReadRegisterList(std::vector<std::string> &reg_list, uint32_t core_id, CoreType core_type);
  virtual Status GetDeviceInfo(DeviceInfo &device_info);
  virtual Status GetCoresInfo(std::vector<CoreInfo> &cores_info);
  virtual Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) = 0;
  virtual Status GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) = 0;
  virtual Status CheckRegisterAddr(CoreType core_type, uint64_t addr) = 0;
  virtual void SetBreakpointCallback(const Callback &callback);

  virtual bool StartListenThread();
  virtual Status EnableDebugMode();
  virtual Status InvalidInstrCache(const lldb::addr_t &addr, 
                                   const InterruptPosInfo &pos_info, uint8_t redirect_ifu = 0) const;
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
  virtual MemType GetStackMemType() const = 0;
  Status BaseSqCqComm(CmdType type, const uint8_t *data = nullptr, const uint8_t len = 0,
                      uint8_t *out = nullptr, uint8_t out_len = 0) const;
  void SetSocket(const Socket *client_socket);
  bool IsDeviceIdMatched(const uint32_t device_id) const;
  bool IsTgidMatched(const int32_t tgid) const;
  void UpdateTgid(const int32_t tgid);

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

private:
  lldb::thread_result_t RunListenThread();
};

} // namespace lldb_private

#endif // #ifndef liblldb_DEVICE_CONTEXT_
#endif // #ifdef MS_DEBUGGER
