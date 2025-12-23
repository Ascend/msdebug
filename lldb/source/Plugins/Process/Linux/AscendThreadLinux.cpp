/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include <thread>

#include "AscendThreadLinux.h"

#include "AscendProcessLinux.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Host/HostNativeThread.h"
#include "lldb/Host/linux/Ptrace.h"
#include "lldb/Utility/LLDBLog.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::process_linux;

static auto g_wait_duration = std::chrono::milliseconds(100);

AscendThreadLinux::AscendThreadLinux(AscendProcessLinux &process,
                                     lldb::tid_t tid)
    : NativeThreadLinux(*static_cast<NativeProcessLinux*>(&process), tid) {}

Status AscendThreadLinux::Resume(uint32_t signo) {
  Log *const log = GetLog(POSIXLog::Thread);
  LLDB_LOG(log, "AscendThreadLinux::{0}, tid: {1}", __FUNCTION__, GetID());
  if (GetStopInfo().still_break_in_device) {
    const StateType new_state = StateType::eStateRunning;
    MaybeLogStateChange(new_state);
    m_state = new_state;
    m_stop_info.reason = StopReason::eStopReasonNone;
    m_stop_info.still_break_in_device = false;
    m_stop_description.clear();
    LLDB_LOG(log, "Use AscendProcessLinux ResumeDevice");
    AscendProcessLinux &process = AscendThreadLinux::GetProcess();
    siginfo_t info;
    const auto info_err = NativeProcessLinux::PtraceWrapper(PTRACE_GETSIGINFO, GetID(), nullptr, &info);

    // If the signal can be obtained, means the host is stopping and need to resume host.
    Status res;
    if (info_err.Success()) {
      res = NativeThreadLinux::Resume(signo);
    }
    process.ResumeDevice();
    return res;
  }
  return NativeThreadLinux::Resume(signo);
}

NativeRegisterContextLinux & AscendThreadLinux::GetRegisterContext() {
  if (m_stop_info.still_break_in_device) {
    if (!m_device_reg_context_up) {
      auto soc_type = GetProcess().GetSocType();
      if (soc_type == SocType::SOC_END) {
        return NativeThreadLinux::GetRegisterContext();
      }
      m_device_reg_context_up =
          NativeRegisterContextLinux::CreateDeviceNativeRegisterContextLinux(
              ArchSpec("hiipu64"), soc_type, *static_cast<NativeThreadLinux*>(this));
    }
    return *m_device_reg_context_up;
  }
  return NativeThreadLinux::GetRegisterContext();
}


AscendProcessLinux &AscendThreadLinux::GetProcess() {
  return static_cast<AscendProcessLinux &>(m_process);
}

const AscendProcessLinux &AscendThreadLinux::GetProcess() const {
  return static_cast<const AscendProcessLinux &>(m_process);
}

void AscendThreadLinux::SetStoppedByDeviceBreakpoint(const InterruptEvent &event) {
  if (m_state == StateType::eStateStepping)
    m_step_workaround.reset();

  const StateType new_state = StateType::eStateStopped;
  MaybeLogStateChange(new_state);
  m_state = new_state;
  m_stop_description.clear();

  m_stop_info.reason = StopReason::eStopReasonDeviceBreakpoint;
  m_stop_info.signo = SIGTRAP;
  m_stop_info.details.device.break_addr = event.pc;
  m_stop_info.details.device.core_id = event.core_id;
  m_stop_info.details.device.core_type = event.core_type;
  m_stop_info.still_break_in_device = true;
}

Status AscendThreadLinux::SingleStep(uint32_t signo) {
  AscendProcessLinux &process = GetProcess();
  if (process.IsDeviceBreak() && process.GetID() == GetID()) {
    return GetProcess().SingleStep();
  }
  return NativeThreadLinux::SingleStep(signo);
}

Status AscendThreadLinux::RequestStop() {
  if (!GetStopInfo().still_break_in_device) {
      return NativeThreadLinux::RequestStop();
  }
  Log *log = GetLog(LLDBLog::Thread);
  LLDB_LOGF(log,
            "AscendThreadLinux::%s requesting thread stop",
            __FUNCTION__);

  return Status();
}

void AscendThreadLinux::SetStopped(bool use_reg) {
  if (m_state == StateType::eStateStepping)
    m_step_workaround.reset();

  // On every stop, clear any cached register data structures
  if (use_reg) {
   GetRegisterContext().InvalidateAllRegisters();
  }

  const StateType new_state = StateType::eStateStopped;
  MaybeLogStateChange(new_state);
  m_state = new_state;
  m_stop_description.clear();
}

void AscendThreadLinux::SetDeviceStoppedByTrace(const InterruptEvent &param) {
  SetStopped();

  m_stop_info.reason = StopReason::eStopReasonTrace;
  m_stop_info.signo = SIGTRAP;
  m_stop_info.details.device.break_addr = param.pc;
  m_stop_info.details.device.core_id = param.core_id;
  m_stop_info.details.device.core_type = param.core_type;
  m_stop_info.details.device.pos_type = static_cast<uint8_t>(param.pos_type);
  m_stop_info.details.device.thread_x = param.thread_x;
  m_stop_info.details.device.thread_y = param.thread_y;
  m_stop_info.details.device.thread_z = param.thread_z;
}

void AscendThreadLinux::SetStoppedByDeviceCtrlC(const InterruptEvent &event) {
  Log *log = GetLog(LLDBLog::Thread);
  LLDB_LOGF(log, "AscendThreadLinux::%s called", __FUNCTION__);
  m_stop_info.reason = StopReason::eStopReasonSignal;
  m_stop_info.signo = SIGSTOP;
  m_stop_info.details.device.break_addr = event.pc;
  m_stop_info.details.device.core_id = event.core_id;
  m_stop_info.details.device.core_type = event.core_type;
  m_stop_info.details.device.pos_type = static_cast<uint8_t>(event.pos_type);
  m_stop_info.details.device.thread_x = event.thread_x;
  m_stop_info.details.device.thread_y = event.thread_y;
  m_stop_info.details.device.thread_z = event.thread_z;
  m_stop_info.still_break_in_device = true;
}

void AscendThreadLinux::SetStoppedBySignal(uint32_t signo, const siginfo_t *info) {
  Log *log = GetLog(LLDBLog::Thread);
  LLDB_LOGF(log, "AscendThreadLinux::%s called with signal 0x%02" PRIx32,
            __FUNCTION__, signo);

  NativeThreadLinux::SetStoppedBySignal(signo, info);

  if (signo != SIGSTOP) {
    return;
  }

  Status error;
  AscendProcessLinux &process = GetProcess();
  if (process.GetID() == GetID() && info) {
    error = process.TaskKill();
    if (error.Fail()) {
      LLDB_LOG(log, "AscendThreadLinux::{0} error={1}", __FUNCTION__, error);
      return;
    }
  }
}

#endif
