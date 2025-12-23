/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef liblldb_AscendProcessLinux_H_
#define liblldb_AscendProcessLinux_H_

#include "NativeProcessLinux.h"
#include "DeviceContext/DeviceContext.h"
#include "AscendCommunicationServer.h"
#define VEC_SIZE 4
namespace lldb_private {


namespace process_linux {
class AscendThreadLinux;

class AscendProcessLinux : public NativeProcessLinux {
public:
  void ResumeDevice();
  Status SetBreakpoint(lldb::addr_t addr, uint32_t size, llvm::Triple::ArchType arch_type, bool hardware) override;

  Status RemoveBreakpoint(lldb::addr_t addr, llvm::Triple::ArchType arch_type, bool hardware = false) override;

  explicit AscendProcessLinux(::pid_t pid, int terminal_fd, NativeDelegate &delegate,
          const ArchSpec &arch, Manager &manager,
          llvm::ArrayRef<::pid_t> tids, const std::string& socket_path = "");
  virtual ~AscendProcessLinux();
  void HandleProcessState(const DebugRecvInfo &info);
  AscendThreadLinux* GetThreadByID(lldb::tid_t tid);
  AscendThreadLinux* GetCurrentThread();
  Status Resume(const ResumeActionList &resume_actions) override;
  void MonitorTrace(const InterruptEvent &param);
  Status SingleStep();
  void SetAicOnFocus(const uint32_t &core_id) override;
  void SetAivOnFocus(const uint32_t &core_id) override;
  void SetSingleCoreRunFlag(bool isSingleCoreRun) override;
  void SetLoadedKernelHash(const std::string &kernel_hash) override;
  void SetClientDeviceId(const int32_t device_id) override;
  Status GetDeviceInfo(DeviceInfo &info) override;
  Status GetCoresInfo(std::vector<CoreInfo> &info) override;
  Status GetCoreInfo(const uint32_t &idx, CoreInfo &info, bool flush_cache = false) override;
  Status GetStoppedCorePC(lldb::addr_t &pc) override;
  Status GetKernelInfo(KernelInfo &info) override;
  lldb::addr_t GetBasePC() override;

  Status ReadDeviceRegisterValue(uint32_t reg_num, uint64_t &value) override;
  Status ReadDeviceRegisterValue(const llvm::StringRef reg_name, uint64_t &value) override;
  Status ReadDeviceRegisterList(std::vector<std::string> &reg_list) override;
  Status ReadDeviceRegisterValue(const RegisterInfo *reg_info, RegisterValue &value);

  Status ReadMemoryWithoutTrap(lldb::addr_t addr, void *buf, size_t size, size_t &bytes_read,
                               const MemoryReaderParamForServer &param) override;
  void MonitorSignal(const InterruptEvent &param);
  Status TaskKill();

  SocType GetSocType() override {
    if (m_device_context) {
      return m_device_context->GetSocType();
    }
    return SocType::SOC_END;
  }

private:
  void MonitorBreakpoint(const InterruptEvent &param);
  Status InitDeviceContext(const int device_id, const std::string &soc_version, const pid_t tgid);
  Status SetDeviceSoftwareBreakpoint(lldb::addr_t addr);
  Status SetDeviceHardwareBreakpoint(lldb::addr_t addr);
  Status SetSwBpByMemory(lldb::addr_t addr, llvm::ArrayRef<uint8_t> expected_trap,
                         llvm::SmallVector<uint8_t, VEC_SIZE>&);
  Status RemoveSwBpByMemory(lldb::addr_t addr, const SoftwareBreakpoint &bp);

  AscendThreadLinux& AddThread(lldb::tid_t thread_id, bool resume);

  Status RemoveDeviceHardwareBreakpoint(lldb::addr_t addr);
  Status RemoveDeviceSoftwareBreakpoint(lldb::addr_t addr);
  Status ReadDeviceMemory(lldb::addr_t addr, void *buf, size_t size, size_t &bytes_read);
  Status WriteDeviceMemory(lldb::addr_t addr, const void *buf, size_t size, size_t &bytes_written);

  llvm::Expected<NativeProcessProtocol::SoftwareBreakpoint>
  EnableDeviceSoftwareBreakpoint(lldb::addr_t addr);

  void RegisterParsers();
  void HandleMsg(Socket *client_socket, const std::string &msg);
  Status HandleStubDeviceInfo(const DeviceInfoMsg& device_info_msg);
  Status HandleStubKernelInfo(const KernelInfoMsg& kernel_info_msg);
  Status HandleStreamId(uint32_t stream_id);

  std::shared_ptr<AscendCommunicationServer> m_server;
  const Socket *m_client_socket = nullptr;
  int m_socket_fd;
  InterruptPosInfo m_pos_info{};
  uint16_t m_thread_dim[3]{};
  lldb::addr_t m_base_pc;
  std::shared_ptr<DeviceContext> m_device_context;
  std::vector<CoreInfo> m_cores_info;
  std::vector<std::function<Status(void)>> m_lazy_calls;
  std::string m_kernel_name;
  uint32_t m_stream_id {};
  pid_t m_tgid {0};
  std::string m_kernel_hash_stub;
  std::string m_kernel_hash_loaded;
  int32_t m_client_device_id {-1};
  bool m_is_handle_pc {false};
  std::set<uint32_t> m_device_ids;
  MsgParser m_parser;
  std::mutex m_socket_mutex;
  std::unordered_map<const Socket*, std::pair<int32_t, pid_t>> m_socket_device_pid;
};

} // namespace process_linux
} // namespace lldb_private

#undef VEC_SIZE
#endif // #ifndef liblldb_AscendProcessLinux_H_
#endif
