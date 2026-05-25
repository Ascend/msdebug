/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef LLDB_SOURCE_PLUGINS_DYNAMICLOADER_POSIX_DYLD_INSTRUCTION_MATCHER_H
#define LLDB_SOURCE_PLUGINS_DYNAMICLOADER_POSIX_DYLD_INSTRUCTION_MATCHER_H

#include <cstdint>
#include <memory>

#include "lldb/Target/Process.h"
#include "lldb/Breakpoint/StoppointCallbackContext.h"

class VFBreakpointHitCallback {
public:
  VFBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) : m_encoding(encoding),
      m_process(process) {}

  virtual bool BreakpointHit() = 0;

protected:
  uint64_t GetBitsAsUint64(uint8_t start, uint8_t end);

  lldb::RegisterContextSP GetRegisterContext();

protected:
  uint64_t m_encoding;
  lldb_private::Process *m_process{};
};

class InstructionMatcher {
public:
  InstructionMatcher() = default;

  virtual std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const = 0;

  bool IsMatched(uint64_t encoding) const { return (encoding & m_mask) == m_expected_value; }

  void SetBits(uint8_t start, uint8_t end, uint64_t value) {
    uint8_t num_bits = end - start + 1;
    uint64_t field_mask = ((1ULL << num_bits) - 1) << start;
    m_mask |= field_mask;
    m_expected_value |= (value << start) & field_mask;
  }

protected:
  uint64_t m_mask{};
  uint64_t m_expected_value{};
};

class VFIMatcher: public InstructionMatcher {
public:
  VFIMatcher();

  std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const override;
};

class VFMatcher: public InstructionMatcher {
public:
  VFMatcher();

  std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const override;
};

class VFSimtMatcher: public InstructionMatcher {
public:
  VFSimtMatcher();

  std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const override;
};

class VFIRuMatcher: public InstructionMatcher {
public:
  VFIRuMatcher();

  std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const override;
};

class VFRuMatcher: public InstructionMatcher {
public:
  VFRuMatcher();

  std::shared_ptr<VFBreakpointHitCallback>
  CreateBreakpointHitCallback(uint64_t encoding, lldb_private::Process *process) const override;
};

bool RendezvousVFCallBreakpointHit(
    void *baton, lldb_private::StoppointCallbackContext *context, lldb::user_id_t break_id,
    lldb::user_id_t break_loc_id);

const std::vector<std::shared_ptr<InstructionMatcher>> &GetMatcherList();

#endif // LLDB_SOURCE_PLUGINS_DYNAMICLOADER_POSIX_DYLD_INSTRUCTION_MATCHER_H
#endif // MS_DEBUGGER
