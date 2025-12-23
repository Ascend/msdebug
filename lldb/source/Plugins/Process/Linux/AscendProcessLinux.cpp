/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "AscendProcessLinux.h"

#include <fcntl.h>

#include "AscendThreadLinux.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Host/posix/ConnectionFileDescriptorPosix.h"
#include "lldb/Utility/AscendVerification.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Status.h"
#include "lldb/Utility/StreamString.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::process_linux;
using namespace llvm;
const std::string DEVICE_INFO_HEADER{"$device_id:"};
const std::string KERNEL_INFO_HEADER{"$kernel_name:"};
const std::string STREAM_ID_HEADER{"$stream_id:"};
constexpr uint32_t VEC_SIZE = 4;
constexpr uint32_t RADIX_DEC = 10;
std::condition_variable g_pause_cv;
std::mutex g_pause_mutex;
bool g_paused = false;

enum ASCEND_PROCESS_ERROR_CODE {
    OPEN_KO_ERR = 0x20200,
    SET_BREAKPOINT_ERR = 0x20201,
    INIT_DEBUG_MODE_ERR = 0x20202,
    INTERNAL_DEBUGGER_ERR = 0x20203,
    UNSUPPORTED_SOC_TYPE_ERR = 0x20204
};

AscendProcessLinux::AscendProcessLinux(::pid_t pid, int terminal_fd, NativeDelegate &delegate,
                                       const ArchSpec &arch, Manager &manager,
                                       llvm::ArrayRef<::pid_t> tids, const std::string& socket_path)
    : NativeProcessLinux(pid, terminal_fd, delegate, arch, manager, tids) {
  Status status;
  RegisterParsers();
  if (m_server == nullptr) {
    static constexpr size_t MAX_CLIENT_NUM = 1024;
    m_server = std::make_shared<AscendCommunicationServer>(MAX_CLIENT_NUM, socket_path);
    auto func = std::bind(&AscendProcessLinux::HandleMsg, this, std::placeholders::_1, std::placeholders::_2);
    m_server->SetMsgHandlerHook(func);
    m_server->Start();
  }
  m_base_pc = 0;
  m_pos_info.core_type = CoreType::UNKNOWN_CORE_TYPE;
  m_pos_info.core_id = 0;
  m_device_context.reset();

  m_cores_info.clear();
  m_lazy_calls.clear();
  m_kernel_name.clear();

  for (const auto &tid : tids) {
    AscendThreadLinux &thread = AddThread(tid, false);
    ThreadWasCreated(thread);
  }

  // Let our process instance know the thread has stopped.
  SetCurrentThreadID(tids[0]);
  SetState(StateType::eStateStopped, false);
  // Proccess any signals we received before installing our handler
  /* SigchldHandler(); */
}

AscendProcessLinux::~AscendProcessLinux() = default;

Status AscendProcessLinux::InitDeviceContext(const int device_id, const std::string &soc_version, const ::pid_t tgid) {
  Log *log = GetLog(POSIXLog::Process);
  Status error;
  LLDB_LOG(log, "Start create device context");
  m_device_context = DeviceContext::Factory::GetDeviceContext(soc_version, tgid, device_id);
  if (!m_device_context) {
    error.SetError(ASCEND_PROCESS_ERROR_CODE::UNSUPPORTED_SOC_TYPE_ERR, lldb::eErrorTypeGeneric);
    error.SetErrorStringWithFormatv("Get device context failed: soc_version={0}\n", soc_version);
    return error;
  }
  error = m_device_context->Init();
  if (error.Fail()) {
    error.SetError(ASCEND_PROCESS_ERROR_CODE::OPEN_KO_ERR, lldb::eErrorTypeGeneric);
    error.SetErrorStringWithFormatv("Init device context failed: {0}", error);
    return error;
  }
  m_device_context->SetBreakpointCallback([this](const DebugRecvInfo &info) {HandleProcessState(info);});
  error = m_device_context->EnableDebugMode();
  if (error.Fail()) {
    error.SetError(ASCEND_PROCESS_ERROR_CODE::INIT_DEBUG_MODE_ERR, lldb::eErrorTypeGeneric);
    error.SetErrorStringWithFormatv("Enable debug mode failed: {0}", error);
    return error;
  }
  if (!m_device_context->StartListenThread()) {
    error.SetError(ASCEND_PROCESS_ERROR_CODE::INTERNAL_DEBUGGER_ERR, lldb::eErrorTypeGeneric);
    error.SetErrorStringWithFormatv("Start listen thread failed");
    return error;
  }
  m_device_context->SetSocket(m_client_socket);
  return error;
}

Status AscendProcessLinux::HandleStubDeviceInfo(const DeviceInfoMsg& device_info_msg) {
  Log *log = GetLog(POSIXLog::Process);
  LLDB_LOG(log, "Process device id");
  Status error;
  m_socket_device_pid[m_client_socket] = std::make_pair(device_info_msg.device_id, device_info_msg.tgid);
  m_tgid = device_info_msg.tgid;
  m_device_ids.insert(device_info_msg.device_id);
  if (m_device_context) {
    LLDB_LOG(log, "device context has been constructed");
    return error;
  }
  if (m_client_device_id == -1) {
    LLDB_LOG(log, "client do not set device id, use the first device id: {0}", device_info_msg.device_id);
    error = InitDeviceContext(device_info_msg.device_id, device_info_msg.soc_version, m_tgid);
  } else if (m_client_device_id == device_info_msg.device_id) {
    LLDB_LOG(log, "client set device id, and it is matched, use {0} to init device context", m_client_device_id);
    error = InitDeviceContext(m_client_device_id, device_info_msg.soc_version, m_tgid);
  } else {
    LLDB_LOG(log, "device id do not match, client device id: {0}, set device id: {1}", m_client_device_id,
             device_info_msg.device_id);
  }
  return error;
}

Status AscendProcessLinux::HandleStubKernelInfo(const KernelInfoMsg& kernel_info_msg) {
  Log *log = GetLog(POSIXLog::Process);
  Status final_error;
  do {
    if (m_device_context == nullptr) {
      LLDB_LOG(log, "{0} device context is not initialized!", __FUNCTION__);
      break;
    }
    if (!m_device_context->IsSocketMatched(m_client_socket)) {
      LLDB_LOG(log, "client is not matched");
      break;
    }
    LLDB_LOG(log, "Process kernel info");
    if (kernel_info_msg.kernel_hash != m_kernel_hash_loaded) {
      LLDB_LOG(log, "kernel name: {0}, kernel hash is not matched, stub kernel "
                    "hash: {1}, loaded kernel hash: {2}, do not process pc",
                    m_kernel_name, m_kernel_hash_stub, m_kernel_hash_loaded);
      break;
    }
    if (m_is_handle_pc) {
      LLDB_LOG(log, "has already processed pc");
      break;
    }
    m_kernel_name = kernel_info_msg.kernel_name;
    m_base_pc = kernel_info_msg.pc_base_addr;
    LLDB_LOGF(log, "success get base_pc=%#lx", m_base_pc);
    m_device_context->SetCanRun(false);
    for (const auto &func: m_lazy_calls) {
      final_error = func();
      if (final_error.Fail()) {
        LLDB_LOG(log, "Lazy call failed: reason: {0}", final_error);
      }
    }
    // the lazy calls functions run only once, need to clear
    if (!final_error.Fail()) {
      m_lazy_calls.clear();
      m_is_handle_pc = true;
    }
    m_device_context->SetCanRun(true);
  } while (0);

  return final_error;
}

Status AscendProcessLinux::HandleStreamId(uint32_t stream_id) {
  Log *log = GetLog(POSIXLog::Process);
  if (m_device_context == nullptr) {
    LLDB_LOG(log, "{0} device context is not initialized!", __FUNCTION__);
    return {};
  }
  if (!m_device_context->IsSocketMatched(m_client_socket)) {
    LLDB_LOG(log, "client is not matched");
    return {};
  }
  LLDB_LOG(log, "Process stream id");
  m_stream_id = stream_id;
  return {};
}

void AscendProcessLinux::RegisterParsers() {
  m_parser.Register(
    DEVICE_INFO_HEADER,
    "\\$device_id:(\\d+);tgid:(\\d+);soc_version:([^;]+);",
    std::make_shared<DeviceHandler>([this](const DeviceInfoMsg& msg) {
        return HandleStubDeviceInfo(msg);
    })
  );

  m_parser.Register(
    KERNEL_INFO_HEADER,
    "\\$kernel_name:([^;]+);kernel_hash:([^;]+);pc_base_addr:([\\d;]+);",
    std::make_shared<KernelHandler>([this](const KernelInfoMsg& msg) {
        return HandleStubKernelInfo(msg);
    })
  );

  m_parser.Register(
    STREAM_ID_HEADER,
    "\\$stream_id:(\\d+);",
    std::make_shared<StreamHandler>([this](uint32_t stream_id) {
        return HandleStreamId(stream_id);
    })
  );
}

void AscendProcessLinux::HandleMsg(Socket *client_socket, const std::string &msg) {
  Log *log = GetLog(POSIXLog::Process);
  if (m_socket_device_pid.find(client_socket) == m_socket_device_pid.end()) {
    LLDB_LOG(log, "receive this device id for the fisrt time, message is: {0}", msg);
  } else {
    LLDB_LOG(log, "receive message for device id: {0}, pid: {1}, message is: {2}",
      m_socket_device_pid[client_socket].first, m_socket_device_pid[client_socket].second, msg);
  }
  // 让这个处理函数变成串行, 因为很多地方用到了m_client_socket，并行逻辑会乱
  std::lock_guard<std::mutex> guard(m_socket_mutex);
  m_client_socket = client_socket;
  Status error = m_parser.ParseMessage(msg);
  std::string reply = "$ok;";
  if (error.Fail()) {
    StreamString temp;
    temp.Printf("$fail:%x;", error.GetError());
    reply = temp.GetString().data();
  }
  size_t send_bytes = reply.size();
  if (client_socket->Write(reply.data(), send_bytes).Fail()) {
    LLDB_LOG(log, "reply failed");
  }
}

AscendThreadLinux &AscendProcessLinux::AddThread(lldb::tid_t thread_id,
                                                 bool resume) {
  Log *log = GetLog(POSIXLog::Thread);
  LLDB_LOG(log, "pid {0} adding thread with tid {1}", GetID(), thread_id);

  assert(!HasThreadNoLock(thread_id) &&
         "attempted to add a thread by id that already exists");

  // If this is the first thread, save it as the current thread
  if (m_threads.empty())
    SetCurrentThreadID(thread_id);

  m_threads.push_back(std::make_unique<AscendThreadLinux>(*this, thread_id));
  AscendThreadLinux &thread =
      static_cast<AscendThreadLinux &>(*m_threads.back());

  Status tracing_error = NotifyTracersOfNewThread(thread.GetID());
  if (tracing_error.Fail()) {
    thread.SetStoppedByProcessorTrace(tracing_error.AsCString());
    StopRunningThreads(thread.GetID());
  } else if (resume)
    ResumeThread(thread, eStateRunning, LLDB_INVALID_SIGNAL_NUMBER);
  else
    thread.SetStoppedBySignal(SIGSTOP);

  return thread;
}

Status AscendProcessLinux::SetBreakpoint(lldb::addr_t addr, uint32_t size,
                                         llvm::Triple::ArchType arch_type, bool hardware) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  LLDB_LOG(log, "AscendProcessLinux::{0} addr = {1:x}", __FUNCTION__, addr);
  if (arch_type == llvm::Triple::hiipu64) {
    if (hardware) {
      return SetDeviceHardwareBreakpoint(addr);
    } else {
      return SetDeviceSoftwareBreakpoint(addr);
    }
  } else {
    return NativeProcessLinux::SetBreakpoint(addr, size, hardware);
  }
}

Status AscendProcessLinux::RemoveBreakpoint(
    lldb::addr_t addr, llvm::Triple::ArchType arch_type, bool hardware) {
  if (arch_type == llvm::Triple::hiipu64) {
    if (hardware) {
      return RemoveDeviceHardwareBreakpoint(addr);
    } else {
      return RemoveDeviceSoftwareBreakpoint(addr);
    }
  }
  return NativeProcessLinux::RemoveBreakpoint(addr);
}

Status AscendProcessLinux::RemoveDeviceHardwareBreakpoint(lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  if (m_base_pc == 0) {
    LLDB_LOG(log, "save lazy call for {0} with addr = {1:x}", __FUNCTION__, addr);
    auto func = [this, addr]() -> Status { return RemoveDeviceHardwareBreakpoint(addr); };
    m_lazy_calls.push_back(func);
    return Status();
  }
  Status error = m_device_context->RemoveHardwareBreakpoint(addr, m_stream_id, m_pos_info);
  if (error.Fail()) {
    return error;
  }
  size_t result = m_hw_breakpoints_map.erase(addr);
  if (result == 0) {
    error.SetErrorString("erase in m_hw_breakpoints_map failed, addr does not exist");
  }
  return error;
}

Status AscendProcessLinux::RemoveDeviceSoftwareBreakpoint(lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  if (m_base_pc == 0) {
    LLDB_LOG(log, "save lazy call for {0} with addr = {1:x}", __FUNCTION__, addr);
    auto func = [this, addr]() -> Status { return RemoveDeviceSoftwareBreakpoint(addr); };
    m_lazy_calls.push_back(func);
    return Status();
  }
  Status error;
  if (m_base_pc == 0) {
    error.SetErrorString("base_pc=0");
    return error;
  }
  LLDB_LOG(log, "addr = {0:x}", addr);
  auto it = m_software_breakpoints.find(addr);
  if (it == m_software_breakpoints.end())
    return Status("Breakpoint not found.");
  assert(it->second.ref_count > 0);
  if (--it->second.ref_count > 0)
    return Status();

  if (m_device_context->SupportDirectlySetSwBp()) {
    m_device_context->RemoveSoftwareBreakpoint(addr);
  } else {
    error = RemoveSwBpByMemory(addr, it->second);
    if (error.Fail()) {
      return error;
    }
  }
  m_software_breakpoints.erase(it);
  return error;
}


Status AscendProcessLinux::Resume(const ResumeActionList &resume_actions) {
  if (!IsDeviceBreak()) {
    return NativeProcessLinux::Resume(resume_actions);
  }
  Log *log = GetLog(POSIXLog::Process);
  LLDB_LOG(log, "pid {0}", GetID());
  const auto &thread = GetCurrentThread();
  if (!thread) {
    return Status("AscendProcessLinux::%s (): can not get current thread.",
                  __FUNCTION__);
  }
  const ResumeAction *const action =
      resume_actions.GetActionForThread(thread->GetID(), true);
  if (!action) {
    return Status("AscendProcessLinux::%s (): can not get action.",
                  __FUNCTION__);
  }

  switch (action->state) {
    case eStateRunning:
    case eStateStepping: {
      const int signo = action->signal;
      ResumeThread(static_cast<NativeThreadLinux &>(*thread), action->state,
                   signo);
      // make sure we finished resume thread response then start receive event from driver
      m_device_context->SetCanRun(true);
      break;
    }
    case eStateSuspended:
    case eStateStopped:
      break;
    default:
      return Status("AscendProcessLinux::%s (): unexpected state %d",
                    __FUNCTION__, action->state);
  }
  return Status();
}

Status AscendProcessLinux::SetDeviceHardwareBreakpoint(lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  if (m_base_pc == 0) {
    LLDB_LOG(log, "save lazy call for {0} with addr = {1:x}", __FUNCTION__, addr);
    auto func = [this, addr]() -> Status { return SetDeviceHardwareBreakpoint(addr); };
    m_lazy_calls.push_back(func);
    return Status();
  }
  addr += m_base_pc;
  return m_device_context->SetHardwareBreakpoint(addr, m_stream_id, m_pos_info);
}

AscendThreadLinux *AscendProcessLinux::GetThreadByID(lldb::tid_t tid) {
  return static_cast<AscendThreadLinux *>(
      NativeProcessProtocol::GetThreadByID(tid));
}

AscendThreadLinux *AscendProcessLinux::GetCurrentThread() {
  return static_cast<AscendThreadLinux *>(
      NativeProcessProtocol::GetCurrentThread());
}


Status AscendProcessLinux::SetDeviceSoftwareBreakpoint(lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  if (m_base_pc == 0) {
    auto func = [this, addr]() -> Status { return SetDeviceSoftwareBreakpoint(addr); };
    m_lazy_calls.push_back(func);
    return Status();
  }
  LLDB_LOG(log, "addr = {0:x}", addr);

  auto it = m_software_breakpoints.find(addr);
  if (it != m_software_breakpoints.end()) {
    ++it->second.ref_count;
    return Status();
  }

  auto expected_bkpt = EnableDeviceSoftwareBreakpoint(addr);
  if (!expected_bkpt)
    return Status(expected_bkpt.takeError());

  m_software_breakpoints.emplace(addr, std::move(*expected_bkpt));
  return Status();
}

llvm::Expected<NativeProcessProtocol::SoftwareBreakpoint> AscendProcessLinux::EnableDeviceSoftwareBreakpoint(
    lldb::addr_t addr) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  Status error;
  if (m_device_context == nullptr) {
    error.SetErrorString("device context is null!");
    return error.ToError();
  }
  static const uint8_t g_ascend_opcode[] = {0x0, 0x0, 0x80, 0x41};
  ArrayRef<uint8_t> expected_trap = llvm::ArrayRef(g_ascend_opcode);
  llvm::SmallVector<uint8_t, VEC_SIZE> saved_opcode_bytes(expected_trap.size(), 0);
  if (m_device_context->SupportDirectlySetSwBp()) {
    error = m_device_context->SetSoftwareBreakpoint(addr);
  } else {
    error = SetSwBpByMemory(addr, expected_trap, saved_opcode_bytes);
  }
  if (error.Fail()) {
    return error.ToError();
  }
  LLDB_LOG(log, "addr = {0:x}: SUCCESS", addr);
  return SoftwareBreakpoint{1, saved_opcode_bytes, expected_trap};
}

Status AscendProcessLinux::ReadDeviceMemory(lldb::addr_t addr, void *buf, size_t size,
                                            size_t &bytes_read) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  bytes_read = m_device_context->ReadGlobalMemory(addr, size, buf);
  if (bytes_read < size) {
    return Status("addr=0x%" PRIx64
                  ": tried to read device %zu bytes but only read %zu",
                  addr, size, bytes_read);
  }
  return Status();
}

Status AscendProcessLinux::SetSwBpByMemory(lldb::addr_t addr,
                                           ArrayRef<uint8_t> expected_trap,
                                           SmallVector<uint8_t, VEC_SIZE> &saved_opcode_bytes) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  LLDB_LOG(log, "{0}", __FUNCTION__);
  size_t bytes_read = 0;
  Status error;
  if (m_base_pc == 0) {
    error.SetErrorString("base_pc=0");
    return error;
  }
  if (m_device_context == nullptr) {
    error.SetErrorString("device context is null!");
    return error;
  }
  addr += m_base_pc;
  error = ReadDeviceMemory(addr, saved_opcode_bytes.data(), saved_opcode_bytes.size(), bytes_read);
  if (error.Fail()) {
    error.SetErrorStringWithFormat(
      "while attempting to set device breakpoint, got error: %s", error.AsCString());
    return error;
  }

  LLDB_LOG(log, "Overwriting bytes at device addr {0:x}: {1:@[x]}", addr,
           llvm::make_range(saved_opcode_bytes.begin(), saved_opcode_bytes.end()));
  // Write a software breakpoint in place of the original opcode.
  size_t bytes_written = 0;
  error = WriteDeviceMemory(addr, expected_trap.data(), expected_trap.size(), bytes_written);
  if (error.Fail()) {
    error.SetErrorStringWithFormat(
      "while attempting to set device breakpoint, got error: %s", error.AsCString());
    return error;
  }

  llvm::SmallVector<uint8_t, VEC_SIZE> verify_bp_opcode_bytes(expected_trap.size(), 0);
  size_t verify_bytes_read = 0;
  error = ReadDeviceMemory(addr, verify_bp_opcode_bytes.data(),
                           verify_bp_opcode_bytes.size(), verify_bytes_read);
  if (error.Fail()) {
    error.SetErrorStringWithFormat(
      "while attempting to verify device breakpoint, got error: %s", error.AsCString());
    return error;
  }
  // check read data with opcode data
  if (llvm::ArrayRef(verify_bp_opcode_bytes.data(), verify_bytes_read) != expected_trap) {
    error.SetErrorStringWithFormat("Verification of software breakpoint "
      "writing failed - trap opcodes not successfully read back "
      "after writing when setting breakpoint at %#lx", addr);
    return error;
  }
  return m_device_context->InvalidInstrCache(addr, m_pos_info, 1);
}

Status AscendProcessLinux::RemoveSwBpByMemory(lldb::addr_t addr, const SoftwareBreakpoint &bp) {
  Log *log = GetLog(LLDBLog::Breakpoints);
  LLDB_LOG(log, "{0}", __FUNCTION__);
  Status error;
  if (m_base_pc == 0) {
    error.SetErrorString("base_pc=0");
    return error;
  }
  if (m_device_context == nullptr) {
    error.SetErrorString("device context is null!");
    return error;
  }
  addr += m_base_pc;
  llvm::SmallVector<uint8_t, VEC_SIZE> curr_break_op(bp.breakpoint_opcodes.size(), 0);
  size_t bytes_read = 0;
  error = ReadDeviceMemory(addr, curr_break_op.data(), curr_break_op.size(), bytes_read);
  if (error.Fail()) {
    return error;
  }

  const auto &saved = bp.saved_opcodes;
  size_t verify_bytes_read = 0;
  if (ArrayRef(curr_break_op) != bp.breakpoint_opcodes) {
    if (curr_break_op != bp.saved_opcodes)
      return Status("Original breakpoint trap is no longer in memory.");
    LLDB_LOG(log, "Saved device opcodes ({0:@[x]}) have already been restored at {1:x}.",
             llvm::make_range(saved.begin(), saved.end()), addr);
  } else {
    size_t bytes_written = 0;
    error = WriteDeviceMemory(addr, saved.data(), saved.size(), bytes_written);
    if (error.Fail()) {
      return error;
    }
    error = m_device_context->InvalidInstrCache(addr, m_pos_info, 1);
    if (error.Fail()) {
      return error;
    }

    llvm::SmallVector<uint8_t, VEC_SIZE> verify_opcode(saved.size(), 0);
    error = ReadDeviceMemory(addr, verify_opcode.data(), verify_opcode.size(), verify_bytes_read);
    if (error.Fail()) {
      error.SetErrorStringWithFormat("when verify, got error: %s", error.AsCString());
      return error;
    }
    if (verify_opcode != saved)
      LLDB_LOG(log, "Restoring device bytes at {0:x}: {1:@[x]}", addr, llvm::make_range(saved.begin(), saved.end()));
  }
  return Status();
}

Status AscendProcessLinux::WriteDeviceMemory(lldb::addr_t addr, const void *buf,
                                             size_t size, size_t &bytes_written) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  bytes_written = m_device_context->WriteGlobalMemory(addr, size, buf);
  if (bytes_written < size) {
    return Status("addr=0x%" PRIx64
                  ": tried to write device %zu bytes but only wrote %zu",
                  addr, size, bytes_written);
  }
  return Status();
}

void AscendProcessLinux::HandleProcessState(const DebugRecvInfo &info) {
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  Status error;
  if (info.cmd_type == CmdType::INTERRUPT_EVENT) {
    const InterruptEvent *param = (const InterruptEvent*)info.recv_msg;
    LLDB_LOGF(log, "pc=%#lx,core_id=%u,core_status=%d,core_type=%u,pos_type=%u,thread_dim=(%u,%u,%u)",
              param->pc, param->core_id, int(param->status), param->core_type,
              (uint8_t)param->pos_type, param->thread_dim_x, param->thread_dim_y, param->thread_dim_z);
    InterruptEvent event = *param;
    event.pc -= m_base_pc;
    LLDB_LOGF(log, "event pc=%#lx, core_id=%u, core_status=%d",
              event.pc, event.core_id, int(event.status));
    if (param->status == CoreStatus::BRKPT) {
      MonitorBreakpoint(event);
    } else if (param->status == CoreStatus::SINGLE_STEP) {
      MonitorTrace(event);
    } else if (param->status == CoreStatus::TASK_KILLED) {
      MonitorSignal(event);
    }
  }
}

void AscendProcessLinux::MonitorBreakpoint(const InterruptEvent &param) {
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  m_pos_info.core_id = param.core_id;
  m_pos_info.core_type = (CoreType)param.core_type;
  m_pos_info.pos_type = param.pos_type;
  m_pos_info.thread_id_x = param.thread_x;
  m_pos_info.thread_id_y = param.thread_y;
  m_pos_info.thread_id_z = param.thread_z;
  m_thread_dim[0] = param.thread_dim_x;
  m_thread_dim[1] = param.thread_dim_y;
  m_thread_dim[2] = param.thread_dim_z;
  m_cores_info.clear();
  for (const auto &thread_up : m_threads) {
    AscendThreadLinux *thread = GetThreadByID(thread_up->GetID());
    if (!thread) {
      continue;
    }
    if (thread_up->GetID() != GetID()) {
      thread->SetStopped(false);
      continue;
    }
    LLDB_LOG(log, "Handle breakpoint process state, pid = {0}", thread->GetID());
    // Mark the thread as stopped at breakpoint.
    thread->SetStoppedByDeviceBreakpoint(param);
    m_pending_notification_tid = thread->GetID();
    auto &stepping_breakpoint = GetThreadsSteppingWithBreakpoint();
    stepping_breakpoint.clear();
    m_current_thread_id = thread->GetID();
    SetState(StateType::eStateStopped, false);
    m_delegate.ProcessStateChanged(this, StateType::eStateStopped);
    m_pending_notification_tid = LLDB_INVALID_THREAD_ID;
  }
}

void AscendProcessLinux::MonitorTrace(const InterruptEvent &param) {
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  m_cores_info.clear();
  for (const auto &thread_up : m_threads) {
    AscendThreadLinux *thread = GetThreadByID(thread_up->GetID());
    if (!thread) {
      continue;
    }
    if (thread_up->GetID() != GetID()) {
      thread->SetStopped(false);
      continue;
    }
    LLDB_LOG(log, "Handle single step process state, pid = {0}", thread->GetID());
    ((AscendThreadLinux*)thread)->SetDeviceStoppedByTrace(param);
    SetState(StateType::eStateStopped, false);
    m_delegate.ProcessStateChanged(this, StateType::eStateStopped);
  }
}

void AscendProcessLinux::ResumeDevice() {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "{0}", __FUNCTION__);
  if (m_device_context == nullptr) {
    LLDB_LOG(log, "{0} device context is null!", __FUNCTION__);
    return;
  }
  m_device_context->Resume(m_pos_info);
}

Status AscendProcessLinux::SingleStep() {
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "{0}", __FUNCTION__);
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  return m_device_context->SingleStep(m_pos_info);
  return error;
}

void AscendProcessLinux::SetAicOnFocus(const uint32_t &core_id) {
  m_pos_info.core_id = core_id;
  m_pos_info.core_type = CoreType::AIC;
}

void AscendProcessLinux::SetAivOnFocus(const uint32_t &core_id) {
  m_pos_info.core_id = core_id;
  m_pos_info.core_type = CoreType::AIV;
}

void AscendProcessLinux::SetSingleCoreRunFlag(bool isSingleCoreRun) {
  m_pos_info.single_core_run = isSingleCoreRun;
}

void AscendProcessLinux::SetLoadedKernelHash(const std::string &kernel_hash) {
  m_kernel_hash_loaded = kernel_hash;
}

void AscendProcessLinux::SetClientDeviceId(const int32_t device_id) {
  m_client_device_id = device_id;
}

Status AscendProcessLinux::GetDeviceInfo(DeviceInfo &info) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  Status error = m_device_context->GetDeviceInfo(info);
  info.device_ids = m_device_ids;
  return error;
}

Status AscendProcessLinux::GetCoresInfo(std::vector<CoreInfo> &info) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  Status error = m_device_context->GetCoresInfo(info);
  if (error.Fail()) {
    return error;
  }
  m_cores_info = info;
  return error;
}

Status AscendProcessLinux::GetCoreInfo(const uint32_t &idx, CoreInfo &info, bool flush_cache) {
  Status error;
  if (m_cores_info.empty() || flush_cache) {
    error = GetCoresInfo(m_cores_info);
    if (error.Fail()) {
      return error;
    }
  }
  if (idx >= m_cores_info.size()) {
    error.SetErrorStringWithFormatv("out of index: idx={0}, size={1}",
                                    idx, m_cores_info.size());
    return error;
  }
  info = m_cores_info[idx];
  return error;
}

Status AscendProcessLinux::GetStoppedCorePC(addr_t &pc) {
  Status error;
  if (m_cores_info.empty()) {
      error = GetCoresInfo(m_cores_info);
      if (error.Fail()) {
        return error;
      }
  }
  for (const auto &core_info: m_cores_info) {
    if (core_info.core_id == m_pos_info.core_id) {
      pc = core_info.pc - m_base_pc;
      return error;
    }
  }
  error.SetErrorStringWithFormatv("get pc failed: core_id={0}, size={1}",
                                  m_pos_info.core_id, m_cores_info.size());
  return error;
}

addr_t AscendProcessLinux::GetBasePC() {
  return m_base_pc;
}

Status AscendProcessLinux::GetKernelInfo(KernelInfo &info) {
  Status error;
  size_t size;
  if (m_kernel_name.size() >= sizeof(info.name) - 1) {
    size = sizeof(info.name) - 1;
  } else {
    size = m_kernel_name.size();
  }
  m_kernel_name.copy(info.name, size, 0);
  info.name[size] = '\0';
  return error;
}

Status AscendProcessLinux::ReadDeviceRegisterValue(const RegisterInfo *reg_info, RegisterValue &value) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  return m_device_context->ReadRegister(reg_info, m_pos_info.core_id, m_pos_info.core_type, value);
}

Status AscendProcessLinux::ReadDeviceRegisterValue(uint32_t reg_num, uint64_t &value) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  return m_device_context->ReadRegister(reg_num, m_pos_info.core_id, m_pos_info.core_type, value);
}

Status AscendProcessLinux::ReadDeviceRegisterValue(const llvm::StringRef reg_name, uint64_t &value) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  return m_device_context->ReadRegister(reg_name, m_pos_info.core_id, m_pos_info.core_type, value);
}

Status AscendProcessLinux::ReadDeviceRegisterList(std::vector<std::string> &reg_list) {
  if (m_device_context == nullptr) {
    return Status("device context is null!");
  }
  return m_device_context->ReadRegisterList(reg_list, m_pos_info.core_id, m_pos_info.core_type);
}

Status AscendProcessLinux::ReadMemoryWithoutTrap(lldb::addr_t addr, void *buf, size_t size,
    size_t &bytes_read, const MemoryReaderParamForServer &param) {
  if (param.arch_type == llvm::Triple::hiipu64) {
    if (m_device_context == nullptr) {
      return Status("device context is null!");
    }
    Status status;
    if (m_pos_info.core_type == CoreType::UNKNOWN_CORE_TYPE) {
      status.SetErrorString("Core type is unknown, read memory from ascend device failed.");
      return status;
    }
    MemoryTypeInfo memory_type_info{};
    memory_type_info.address_class = param.address_class;
    memory_type_info.element_size = param.element_size;
    bytes_read = m_device_context->ReadMemory(addr, size, memory_type_info, m_pos_info, buf);
    if (bytes_read != size) {
      status.SetErrorString("Read memory from ascend device failed.");
    }
    return status;
  }
  return NativeProcessLinux::ReadMemoryWithoutTrap(addr, buf, size, bytes_read);
}

void AscendProcessLinux::MonitorSignal(const InterruptEvent &param) {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "AscendProcessLinux::{0}, stream_id: {1}", __FUNCTION__, m_stream_id);
  m_pos_info.core_id = param.core_id;
  m_pos_info.core_type = (CoreType)param.core_type;
  m_cores_info.clear();
  LLDB_LOG(log, "AscendProcessLinux::{0}, core_id: {1}, core_type: {2}",
           __FUNCTION__, param.core_id, param.core_type);
  for (const auto &thread_up : m_threads) {
    AscendThreadLinux *thread = GetThreadByID(thread_up->GetID());
    if (thread && thread_up->GetID() == GetID()) {
      thread->SetStoppedByDeviceCtrlC(param);
      std::unique_lock<std::mutex> lk(g_pause_mutex);
      g_paused = true;
      g_pause_cv.notify_all();
    }
  }
}

Status AscendProcessLinux::TaskKill() {
  Status status;
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "AscendProcessLinux::{0}, stream_id: {1}",
           __FUNCTION__, m_stream_id);
  if (m_device_context == nullptr) {
    status.SetErrorString("device context is null!");
    return status;
  }
  status = m_device_context->TaskKill(m_stream_id);
  if (status.Fail()) {
    return status;
  }
  std::unique_lock<std::mutex> lk(g_pause_mutex);
  if (g_pause_cv.wait_for(lk, std::chrono::seconds(1), [=](){return g_paused;})) {
    LLDB_LOG(log, "Paused in device success");
  } else {
    status.SetErrorString("Paused in device failed, timeout!");
  }
  return status;
}
#endif
