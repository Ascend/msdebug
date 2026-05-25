/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "InstructionMatcher.h"


#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Utility/RegisterValue.h"

using namespace lldb_private;
using namespace lldb;

namespace {
class VFICallback : public VFBreakpointHitCallback {
public:
  using VFBreakpointHitCallback::VFBreakpointHitCallback;

  bool BreakpointHit() override {
    int16_t vpc_offset = GetBitsAsUint64(0, 15);
    auto reg_sp = GetRegisterContext();
    if (!reg_sp) {
      return false;
    }
    uint64_t main_scalar_pc = reg_sp->GetPC();
    if (main_scalar_pc == LLDB_INVALID_ADDRESS) {
      return false;
    }
    uint64_t vpc = vpc_offset * 4 + (main_scalar_pc - 4);
    m_process->UpdateVFStartPC(vpc);
    return true;
  }
};

class VFCallback : public VFBreakpointHitCallback {
public:
  using VFBreakpointHitCallback::VFBreakpointHitCallback;

  bool BreakpointHit() override {
    Log *log = GetLog(LLDBLog::DynamicLoader);
    uint8_t reg_x_id = GetBitsAsUint64(16, 20);
    auto reg_sp = GetRegisterContext();
    if (!reg_sp) {
      LLDB_LOG(log, "Get empty register context, stop set vf start pc");
      return false;
    }
    std::string name = "GPR";
    name += std::to_string(static_cast<uint32_t>(reg_x_id));
    LLDB_LOG(log, "Start get register {0} name", name);
    const RegisterInfo *reg_info = reg_sp->GetRegisterInfoByName(name);
    if (!reg_info) {
      LLDB_LOG(log, "Get register {0} info failed", name);
      return false;
    }
    RegisterValue value;
    if (!reg_sp->ReadRegister(reg_info, value)) {
      LLDB_LOG(log, "Get register {0} value failed", name);
      return false;
    }
    uint64_t vpc = value.GetAsUInt64();
    m_process->UpdateVFStartPC(vpc);
    LLDB_LOG(log, "Get vpc value={0:x}", vpc);
    return true;
  }
};
 	 
class VFSimtCallback : public VFBreakpointHitCallback {
public:
  using VFBreakpointHitCallback::VFBreakpointHitCallback;
  // Simt don't need
  bool BreakpointHit() override { return true; }
};
} // namespace

RegisterContextSP VFBreakpointHitCallback::GetRegisterContext() {
  Log *log = GetLog(LLDBLog::DynamicLoader);
  if (!m_process) {
    LLDB_LOG(log, "Get empty m_process, stop get register context");
    return nullptr;
  }

  ThreadSP thread_sp = m_process->GetThreadList().GetSelectedThread();
  if (!thread_sp) {
    LLDB_LOG(log, "Get empty thread, stop get register context");
    return nullptr;
  }
  return thread_sp->GetRegisterContext();
}

uint64_t VFBreakpointHitCallback::GetBitsAsUint64(uint8_t start, uint8_t end) {
  uint8_t num_bits = end - start + 1;
  uint64_t field_mask = (1ULL << num_bits) - 1;
  return (m_encoding >> start) & field_mask;
}

VFIMatcher::VFIMatcher() : InstructionMatcher() {
  SetBits(21, 23, 0b011);
  SetBits(24, 28, 0b10101);
  SetBits(29, 31, 0b000);
  SetBits(32, 36, 0b00100);
  SetBits(53, 55, 0b111);
  SetBits(56, 60, 0b10101);
  SetBits(61, 63, 0b000);
}

std::shared_ptr<VFBreakpointHitCallback>
VFIMatcher::CreateBreakpointHitCallback(uint64_t encoding, Process *process) const {
  return std::make_shared<VFICallback>(encoding, process);
}

VFMatcher::VFMatcher() : InstructionMatcher() {
  SetBits(21, 23, 0b010);
  SetBits(24, 28, 0b10101);
  SetBits(29, 31, 0b000);
  SetBits(32, 36, 0b00101);
  SetBits(53, 55, 0b111);
  SetBits(56, 60, 0b10101);
  SetBits(61, 63, 0b000);
}

std::shared_ptr<VFBreakpointHitCallback>
VFMatcher::CreateBreakpointHitCallback(uint64_t encoding, Process *process) const {
  return std::make_shared<VFCallback>(encoding, process);
}

VFSimtMatcher::VFSimtMatcher() : InstructionMatcher() {
  SetBits(21, 23, 0b001);
  SetBits(24, 28, 0b10101);
  SetBits(29, 31, 0b000);
  SetBits(32, 36, 0b11100);
  SetBits(53, 55, 0b111);
  SetBits(56, 60, 0b10101);
  SetBits(61, 63, 0b000);
}

std::shared_ptr<VFBreakpointHitCallback>
VFSimtMatcher::CreateBreakpointHitCallback(uint64_t encoding, Process *process) const {
  return std::make_shared<VFSimtCallback>(encoding, process);
}

VFIRuMatcher::VFIRuMatcher() : InstructionMatcher() {
  SetBits(21, 23, 0b010);
  SetBits(24, 28, 0b10101);
  SetBits(29, 31, 0b000);
  SetBits(32, 36, 0b00110);
  SetBits(53, 55, 0b111);
  SetBits(56, 60, 0b10101);
  SetBits(61, 63, 0b000);
}

std::shared_ptr<VFBreakpointHitCallback>
VFIRuMatcher::CreateBreakpointHitCallback(uint64_t encoding, Process *process) const {
  // the same as vfi callback
  return std::make_shared<VFICallback>(encoding, process);
}

VFRuMatcher::VFRuMatcher() : InstructionMatcher() {
  SetBits(21, 23, 0b001);
  SetBits(24, 28, 0b10101);
  SetBits(29, 31, 0b000);
  SetBits(32, 36, 0b00111);
  SetBits(53, 55, 0b111);
  SetBits(56, 60, 0b10101);
  SetBits(61, 63, 0b000);
}

std::shared_ptr<VFBreakpointHitCallback>
VFRuMatcher::CreateBreakpointHitCallback(uint64_t encoding, Process *process) const {
  // the same as vf callback
  return std::make_shared<VFCallback>(encoding, process);
}

bool RendezvousVFCallBreakpointHit(
    void *baton, StoppointCallbackContext *context, lldb::user_id_t break_id,
    lldb::user_id_t break_loc_id) {
  if (!baton)
    return false;
  Log *log = GetLog(LLDBLog::DynamicLoader);
  VFBreakpointHitCallback *const callback =
      static_cast<VFBreakpointHitCallback*>(baton);
  LLDB_LOGF(log, "%s called", __FUNCTION__);
  callback->BreakpointHit();
  // Don't stop at internal breakpoints; return false to continue execution.
  return false;
}

const std::vector<std::shared_ptr<InstructionMatcher>> &GetMatcherList() {
  static std::vector<std::shared_ptr<InstructionMatcher>> inst_matchers = {
    std::make_shared<VFIMatcher>(),
    std::make_shared<VFMatcher>(),
    std::make_shared<VFSimtMatcher>(),
    std::make_shared<VFRuMatcher>(),
    std::make_shared<VFIRuMatcher>(),
  };
  return inst_matchers;
}

#endif // MS_DEBUGGER
