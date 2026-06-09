/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER

#include "AscendDisassembler950.h"
#include "llvm/MC/MCInstrDesc.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include <vector>

using namespace std;
using namespace lldb_private;

namespace {

struct Inst {
  uint32_t mask;
  uint32_t value;
  uint32_t byte_size{4};
};

const vector<Inst> &GetBranchInsts() {
  static vector<Inst> insts = {
      {0xFFE20000, 0x40020000},    // JUMP
      {0xFFE20000, 0x40000000},    // JUMPI
      {0xFFFE0000, 0x40220000},    // JUMPC
      {0xFFFE0000, 0x40200000},    // JUMPCI
      {0xFFFE0000, 0x40240000},    // JUMPR
      {0xF8030000, 0x48000000},    // JUMPCMPI_IO
      {0xF8030000, 0x48010000},    // JUMPCMP_IO
      {0xF8030000, 0x48020000},    // JUMPCMPI
      {0xF8030000, 0x48030000},    // JUMPCMP
      {0x3FFE0001, 0x01260000, 8}, // SIMT_BRANCH
      {0x3FFE0001, 0x01A60000, 8}, // BRANCH_IND
      {0xC000013F, 0xC0000013},    // SJUMP
      {0xC000013F, 0xC0000113},    // SJUMPI
      {0xC000003F, 0xC0000034},    // SCBZ
      {0xC000003F, 0xC0000014},    // SCBZI
      {0xC000003F, 0xC0000039},    // SEXT
  };
  return insts;
}

const vector<Inst> &GetCallInsts() {
  static vector<Inst> insts = {
      {0xFFFE0000, 0x40400000},     // CALLI
      {0xFFFE0000, 0x40420000},     // CALL
      {0xFFFE0000, 0x40440000},     // CALLR
      {0x3FFE0001, 0x02260000, 8},  // SIMT_CALL
      {0x3FFE0001, 0x05260001, 16}, // SIMT_CALLI
      {0x3FFE0001, 0x07260000, 8},  // SIMT_CALL_IND
  };
  return insts;
}

const vector<Inst> &GetRetInsts() {
  static vector<Inst> insts = {
      {0xFFE1F000, 0x4061F000},    // RET
      {0xFFE00000, 0x41200000},    // ENDLOOP
      {0xFFE00000, 0x41600000},    // END
      {0xFFE00000, 0x41800000},    // BREAK
      {0x3FFE003D, 0x0626001C, 8}, // SIMT_RET
      {0x3FFE0001, 0x06A60000, 8}, // SIMT_END
      {0xC000003F, 0xC0000017},    // SEND
  };
  return insts;
}

uint32_t GetBit(const uint8_t *opcode_data, const size_t opcode_data_len) {
  if (opcode_data_len < 4) {
    return 0;
  }
  uint32_t data_s = (static_cast<uint32_t>(opcode_data[3]) << 24 |
                     static_cast<uint32_t>(opcode_data[2]) << 16 |
                     static_cast<uint32_t>(opcode_data[1]) << 8 |
                     static_cast<uint32_t>(opcode_data[0]));
  return data_s;
}

int GetInstSize(const uint32_t encoding, const vector<Inst> &insts) {
  for (const auto &inst : insts) {
    if ((encoding & inst.mask) == inst.value) {
      return inst.byte_size;
    }
  }
  return 0;
}

} // namespace

void AscendDisassembler950::GetInstruction(const uint8_t *opcode_data,
                                           const size_t opcode_data_len,
                                           uint64_t &flags,
                                           const InterruptPosType pos_type,
                                           uint64_t &inst_size) {
  Log *log = GetLog(LLDBLog::Process);
  uint32_t encoding = GetBit(opcode_data, opcode_data_len);
  inst_size = 4;
  if (pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
    // 大部分都是>8, 小部分是16 32
    inst_size = 8;
  }
  int sz = 0;
  sz = GetInstSize(encoding, GetBranchInsts());
  if (sz > 0) {
    flags |= 1ULL << (uint32_t)llvm::MCID::Branch;
    inst_size = sz;
  }
  sz = GetInstSize(encoding, GetCallInsts());
  if (sz > 0) {
    flags |= 1ULL << (uint32_t)llvm::MCID::Call;
    inst_size = sz;
  }
  sz = GetInstSize(encoding, GetRetInsts());
  if (sz > 0) {
    flags |= 1ULL << (uint32_t)llvm::MCID::Return;
    inst_size = sz;
  }
  LLDB_LOG(log,
           "opcode_data_len={0}, encoding={1:x}, inst_size={2}, flags={3:x}",
           opcode_data_len, encoding, inst_size, flags);
}

#endif
