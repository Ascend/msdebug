/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER

#include "AscendDisassembler310P.h"
#include "llvm/MC/MCInstrDesc.h"
#include <map>

using namespace lldb_private;
namespace {
enum class InstValue : uint32_t {
  JUMP = 0x40000000,
  JUMPC = 0x40200000,
  JUMPR = 0x40240000,
  JUMPCMP = 0x48000000,
  ENDLOOP = 0x41200000,
  CALL = 0X40400000,
  CALLR = 0x40440000,
  END = 0x41600000,
  BREAK = 0x41800000,
  RETURN = 0x4061f000
};

enum class InstMode : uint32_t {
  JUMP = 0xfffc0000,
  JUMPC = 0xfffc0000,
  JUMPR = 0xfffe0fff,
  JUMPCMP = 0xf8000000,
  ENDLOOP = 0xffffffff,
  CALL = 0xfffc0000,
  CALLR = 0xfffe0fff,
  BREAK = 0xffffffff,
  END = 0xffffffff,
  RETURN = 0xffffffff
};

const std::map<InstValue, std::string> G_INST_NAME_MAP{
    {InstValue::JUMP, "JUMP"},       {InstValue::JUMPC, "JUMPC"},
    {InstValue::JUMPR, "JUMPR"},     {InstValue::JUMPCMP, "JUMPCMP"},
    {InstValue::ENDLOOP, "ENDLOOP"}, {InstValue::CALL, "CALL"},
    {InstValue::CALLR, "CALLR"},     {InstValue::BREAK, "BREAK"},
    {InstValue::END, "END"},         {InstValue::RETURN, "RETURN"},
};

const std::map<InstValue, InstMode> G_BRANCH_MAP{
    {InstValue::JUMP, InstMode::JUMP},
    {InstValue::JUMPC, InstMode::JUMPC},
    {InstValue::JUMPR, InstMode::JUMPR},
    {InstValue::JUMPCMP, InstMode::JUMPCMP},
    {InstValue::ENDLOOP, InstMode::ENDLOOP},
};

const std::map<InstValue, InstMode> G_CALL_MAP{
    {InstValue::CALL, InstMode::CALL}, {InstValue::CALLR, InstMode::CALLR}};

const std::map<InstValue, InstMode> G_RETURN_MAP{
    {InstValue::BREAK, InstMode::BREAK},
    {InstValue::END, InstMode::END},
    {InstValue::RETURN, InstMode::RETURN},
};

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

bool IsTargetInst(const uint8_t *opcode_data, const size_t opcode_data_len,
                  const std::map<InstValue, InstMode> &tar_map) {
  uint32_t data = GetBit(opcode_data, opcode_data_len);
  for (auto iter = tar_map.begin(); iter != tar_map.end(); ++iter) {
    if ((data & (uint32_t)(iter->second)) == (uint32_t)(iter->first)) {
      return true;
    }
  }
  return false;
}

} // namespace

void AscendDisassembler310P::GetInstruction(const uint8_t *opcode_data,
                                            const size_t opcode_data_len,
                                            uint64_t &flags,
                                            const InterruptPosType pos_type,
                                            uint64_t &inst_size) {
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_BRANCH_MAP)
               ? (1 << (uint32_t)llvm::MCID::Branch)
               : 0;
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_CALL_MAP)
               ? (1 << (uint32_t)llvm::MCID::Call)
               : 0;
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_RETURN_MAP)
               ? (1 << (uint32_t)llvm::MCID::Return)
               : 0;
  inst_size = 4;
}

#endif
