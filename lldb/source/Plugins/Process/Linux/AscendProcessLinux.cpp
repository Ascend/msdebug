/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2026. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "AscendProcessLinux.h"

#include <fcntl.h>

#include "AscendThreadLinux.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Host/posix/ConnectionFileDescriptorPosix.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Status.h"
#include "lldb/Utility/StreamString.h"
#include "lldb/Utility/RegisterValue.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::process_linux;
using namespace llvm;
const std::string DEVICE_INFO_HEADER{"device_id:"};
const std::string KERNEL_INFO_HEADER{"kernel_name:"};
constexpr uint32_t VEC_SIZE = 4;

static std::condition_variable g_pause_cv;
static std::mutex g_pause_mutex;
static bool g_paused = false;

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
  m_pos_info.Reset();
  m_device_context.reset();
  m_stop = false;

  m_cores_info.clear();
  m_kernel_name.clear();

  for (const auto &tid : tids) {
    AscendThreadLinux &thread = AddThread(tid, false);
    ThreadWasCreated(thread);
  }

  // Let our process instance know the thread has stopped.
  if (!tids.empty()) {
    SetCurrentThreadID(tids[0]);
  }
  SetState(StateType::eStateStopped, false);
}

AscendProcessLinux::~AscendProcessLinux() {
  m_stop = true;
  m_server->Close();
}

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
  return error;
}

HandleResult AscendProcessLinux::HandleStubDeviceInfo(const DeviceInfoMsg& device_info_msg) {
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
  if (error.Success()) {
    error.SetExpressionErrorWithFormat(ExpressionResults::eExpressionCompleted, "Debugger focus on device %d", device_info_msg.device_id);
  }
  return error;
}

HandleResult AscendProcessLinux::HandleStubKernelInfo(const KernelInfoMsg& kernel_info_msg) {
  Log *log = GetLog(POSIXLog::Process);
  Status final_error;
  do {
    if (m_device_context == nullptr) {
      LLDB_LOG(log, "{0} device context is not initialized!", __FUNCTION__);
      break;
    }
    // query device id from socket handle
    auto it = m_socket_device_pid.find(m_client_socket);
    if (it == m_socket_device_pid.end()) {
      LLDB_LOG(log, "cannot find on which device the kernel {0} should be launched.",
        kernel_info_msg.kernel_name.c_str());
      break;
    }

    auto device_id = it->second.first;
    auto tgid = it->second.second;
    if (!m_device_context->IsDeviceIdMatched(device_id)) {
      LLDB_LOG(log, "device id {0} from client {1} is not matched",
        device_id, m_client_socket);
      break;
    }
    LLDB_LOG(log, "device id matched. device id:{0} client:{1}", device_id, m_client_socket);

    m_kernel_name = kernel_info_msg.kernel_name;
    m_stream_id = kernel_info_msg.stream_id;
    LLDB_LOG(log, "update kernel_name to {0}, stream_id to {1}", m_kernel_name, m_stream_id);
    if (!kernel_info_msg.elf.empty()) {
      DeviceBinaryInfo binary_info;
      binary_info.pc_base_addr = kernel_info_msg.pc_base_addr;
      binary_info.binary = kernel_info_msg.elf;
      if (!m_device_context->IsTgidMatched(tgid)) {
        // Critical: this means a kernel is about to be launched on a new process which is
        // also set to the target device. It happens when the previous process exited and
        // a new one was forked to launch kernels on the same device. At this time we need
        // to notify the client to remove the previously loaded kernel binary and restart
        // resolving device breakpoints.
        binary_info.reset_all_device_binary = 1U;
        m_device_context->UpdateTgid(tgid);
      } else {
        binary_info.reset_all_device_binary = 0U;
      }
      m_device_binary_info_que.push(binary_info);
      LLDB_LOG(log, "success get base_pc={0:x}, kernel_hash={1}", kernel_info_msg.pc_base_addr,
               kernel_info_msg.kernel_hash);
      for (const auto &thread_up : m_threads) {
        AscendThreadLinux *thread = GetThreadByID(thread_up->GetID());
        if (!thread) {
          continue;
        }
        if (thread_up->GetID() != GetID()) {
          thread->SetStopped(false);
          continue;
        }
        LLDB_LOG(log, "Fake internel-breakpoint process state, pid = {0}", thread->GetID());
        // Mark the thread as stopped at breakpoint.
        thread->SetStoppedByInternalBreakpoint();
        m_pending_notification_tid = thread->GetID();
        m_current_thread_id = thread->GetID();
        SetState(StateType::eStateStopped, false);
        m_delegate.ProcessStateChanged(this, StateType::eStateStopped);
        m_pending_notification_tid = LLDB_INVALID_THREAD_ID;
      }
      m_internal_break_done = false;
      m_device_context->SetCanRun(false);
      while(!m_internal_break_done && !m_stop);
      LLDB_LOG(log, "process internal fake break point done");
    }
  } while (0);

  return final_error;
}

void AscendProcessLinux::RegisterParsers() {
  m_parser.Register(
    DEVICE_INFO_HEADER,
    std::make_shared<DeviceHandler>([this](const DeviceInfoMsg& msg) {
        return HandleStubDeviceInfo(msg);
    })
  );

  m_parser.Register(
    KERNEL_INFO_HEADER,
    std::make_shared<KernelHandler>([this](const KernelInfoMsg& msg) {
        return HandleStubKernelInfo(msg);
    })
  );
}

void AscendProcessLinux::HandleMsg(Socket *client_socket, const std::string &msg) {
  Log *log = GetLog(POSIXLog::Process);
  constexpr int display_length = 1024;
  llvm::StringRef display_msg(msg.data(), msg.length());
  if (display_msg.size() > display_length) {
    display_msg = display_msg.slice(0, display_length);
  }
  auto it = m_socket_device_pid.find(client_socket);
  if (it == m_socket_device_pid.end()) {
    LLDB_LOG(log, "receive this device id for the fisrt time, message is: {0}, original length is {1}",
             display_msg, msg.length());
  } else {
    LLDB_LOG(log, "receive message for device id: {0}, pid: {1}, message is: {2}",
             it->second.first, it->second.second, display_msg, msg.length());
  }
  // 让这个处理函数变成串行, 因为很多地方用到了m_client_socket，并行逻辑会乱
  std::lock_guard<std::mutex> guard(m_socket_mutex);
  m_client_socket = client_socket;
  HandleResult result = m_parser.ParseMessage(msg);
  std::string reply = "$ok;";
  if (result.Fail()) {
    StreamString temp;
    temp.Printf("$fail:%x;", result.GetError());
    reply = temp.GetString().data();
  } else if (result.GetMessage().length()) {
    reply += result.GetMessage();
    reply += ";";
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
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  AscendThreadLinux *thread = GetThreadByID(GetID());
  ThreadStopInfo stop_info{};
  std::string description;
  if (thread) {
    thread->GetStopReason(stop_info, description);
  }
  if (!stop_info.still_break_in_device || stop_info.internal_break) {
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
  Status error;
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
  std::lock_guard<std::mutex> guard(m_status_mtx);
  std::shared_ptr<void> defer(nullptr, [this](std::nullptr_t &) {
    if (m_state == lldb::eStateRunning) {
      // when resume host, device may invocate a breakpoint then change all thread status to stop more quickly
      // make sure we finished resume thread response then start receive event from driver
      m_device_context->SetCanRun(true);
      m_internal_break_done = true;
    }
  });
  if (!IsDeviceBreak()) {
    return NativeProcessLinux::Resume(resume_actions);
  }
  Log *log = GetLog(POSIXLog::Process);
  LLDB_LOG(log, "{0} pid {1}", __FUNCTION__, GetID());
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
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  AscendThreadLinux *thread = GetThreadByID(GetID());
  ThreadStopInfo stop_info{};
  std::string description;
  if (thread) {
    thread->GetStopReason(stop_info, description);
  }
  // real stop in device
  if (!stop_info.still_break_in_device || stop_info.internal_break) {
    LLDB_LOG(log, "save lazy call for {0} with addr = {1:x}", __FUNCTION__, addr);
    auto func = [this, addr]() -> Status { return SetDeviceHardwareBreakpoint(addr); };
    m_lazy_calls.push_back(func);
    return Status();
  }
  constexpr size_t bp_size = 4;
  auto error = m_device_context->SetHardwareBreakpoint(addr, m_stream_id, m_pos_info);
  if (error.Success()) {
    // Register new hardware breakpoint into hardware breakpoints map of current
    // process.
    m_hw_breakpoints_map[addr] = {addr, bp_size};
  }
  return error;
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
  if (m_device_context == nullptr) {
    error.SetErrorString("device context is null!");
    return error;
  }
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
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  LLDB_LOG(log, "{0}", __FUNCTION__);
  Status error;
  if (m_device_context == nullptr) {
    error.SetErrorString("device context is null!");
    return error;
  }
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
    LLDB_LOGF(log, "pc=%#lx,core_id=%u,core_status=%d,core_type=%u,pos_type=%u,thread_dim=(%u,%u,%u),thread_id=%u",
              param->pc, param->core_id, int(param->status), param->core_type,
              (uint8_t)param->pos_type, param->thread_info.thread_dim_x, param->thread_info.thread_dim_y, param->thread_info.thread_dim_z,
              param->thread_info.thread_id);
    InterruptEvent event = *param;
    std::lock_guard<std::mutex> guard(m_status_mtx);
    if (param->status == CoreStatus::BRKPT) {
      MonitorBreakpoint(event);
    } else if (param->status == CoreStatus::SINGLE_STEP) {
      MonitorTrace(event);
    } else if (param->status == CoreStatus::TASK_KILLED) {
      MonitorSignal(event);
    }
  }
}

void AscendProcessLinux::MonitorBreakpoint(NativeThreadLinux &thread) {
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  LLDB_LOG(log, "received host breakpoint event, pid = {0}", thread.GetID());
  ThreadStopInfo stop_info;
  std::string description;
  // we btter stop host process in future.
  if (thread.GetStopReason(stop_info, description) && stop_info.still_break_in_device && !stop_info.internal_break) {
    LLDB_LOG(log, "warn: current device process is debugging, ignore host breakpoint event.");
    return;
  }
  // host process will exit, so just ignore internal breakpoint event.
  // otherwise, prrcess will be trap all the time.
  if (stop_info.internal_break) {
    stop_info.internal_break = false;
    m_internal_break_done = true;
  }
  NativeProcessLinux::MonitorBreakpoint(thread);
  // We should monitor all host states to detect device context switches.
  // Currently only breakpoints are handled;
  // full solution requires handling all interrupts and suspending device listening.
  if (m_device_context && thread.GetState() == lldb::eStateStopped) {
    m_device_context->SetCanRun(false);
  } else {
    LLDB_LOG(log, "Can not stop device context detect: after host MonitorBreakpoint, "
             "thread state {0}", thread.GetState());
  }
}

void AscendProcessLinux::MonitorBreakpoint(const InterruptEvent &param) {
  Log *log = GetLog(LLDBLog::Process | LLDBLog::Breakpoints);
  m_pos_info.Update(param);
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
    // clear hardware breakpoints
    for (const auto &func: m_lazy_calls) {
      auto error = func();
      if (error.Fail()) {
        LLDB_LOG(log, "Lazy call failed: reason: {0}", error);
      }
    }
    m_lazy_calls.clear();
    //
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
  m_pos_info.Update(param);
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
  m_pos_info.pc = -1;
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

bool AscendProcessLinux::ConsumeKernelBinary(DeviceBinaryInfo &info) {
  if (m_device_binary_info_que.empty()) {
    return false;
  }
  info = std::move(m_device_binary_info_que.front());
  m_device_binary_info_que.pop();
  return true;
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
    if (core_info.core_id == m_pos_info.core_id && core_info.core_type == m_pos_info.core_type) {
      pc = core_info.pc;
      return error;
    }
  }
  error.SetErrorStringWithFormatv("get pc failed: core_id={0}, size={1}",
                                  m_pos_info.core_id, m_cores_info.size());
  return error;
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
  // pc is -1, not stop in device
  if (m_pos_info.pc == UINT64_MAX) {
    return Status("Current not stop in device");
  }
  Status error;
  if (reg_info->kinds[lldb::eRegisterKindGeneric] == LLDB_REGNUM_GENERIC_PC) {
    uint64_t pc = 0;
    error = GetStoppedCorePC(pc);
    if (error.Fail()) {
      return error;
    }
    value.SetUInt64(pc);
    return error;
  }
  return m_device_context->ReadRegister(reg_info, m_pos_info, value);
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
  m_pos_info.Update(param);
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
