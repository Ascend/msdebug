/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER

#include "AscendDisassemblerHelper.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "llvm/MC/MCInstrDesc.h"

const std::map<InstValue, std::string> G_INST_NAME_MAP {
    {InstValue::JUMP, "JUMP"},
    {InstValue::JUMPC, "JUMPC"},
    {InstValue::JUMPR, "JUMPR"},
    {InstValue::JUMPCMP, "JUMPCMP"},
    {InstValue::ENDLOOP, "ENDLOOP"},
    {InstValue::CALL, "CALL"},
    {InstValue::CALLR, "CALLR"},
    {InstValue::BREAK, "BREAK"},
    {InstValue::END, "END"},
    {InstValue::RETURN, "RETURN"},
};

const std::map<InstValue, InstMode> G_BRANCH_MAP {
    {InstValue::JUMP, InstMode::JUMP},
    {InstValue::JUMPC, InstMode::JUMPC},
    {InstValue::JUMPR, InstMode::JUMPR},
    {InstValue::JUMPCMP, InstMode::JUMPCMP},
    {InstValue::ENDLOOP, InstMode::ENDLOOP},
};

const std::map<InstValue, InstMode> G_CALL_MAP {
    {InstValue::CALL, InstMode::CALL},
    {InstValue::CALLR, InstMode::CALLR}
};

const std::map<InstValue, InstMode> G_RETURN_MAP {
    {InstValue::BREAK, InstMode::BREAK},
    {InstValue::END, InstMode::END},
    {InstValue::RETURN, InstMode::RETURN},
};

void AscendDisassemblerHelper::GetInstruction(const uint8_t* opcode_data,
                                              const size_t opcode_data_len, uint64_t &flags, uint64_t &inst_size)
{
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_BRANCH_MAP) ? (1 << (uint32_t)llvm::MCID::Branch) : 0;
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_CALL_MAP) ? (1 << (uint32_t)llvm::MCID::Call) : 0;
  flags |= IsTargetInst(opcode_data, opcode_data_len, G_RETURN_MAP) ? (1 << (uint32_t)llvm::MCID::Return) : 0;
  inst_size = 4; // inst size is 4 in ascend
}

bool AscendDisassemblerHelper::IsTargetInst(const uint8_t *opcode_data, const size_t opcode_data_len,
                                            const std::map<InstValue, InstMode> &tar_map)
{
  uint32_t data = GetBit(opcode_data, opcode_data_len);
  for (auto iter = tar_map.begin(); iter != tar_map.end(); ++iter) {
    if ((data & (uint32_t)(iter->second)) == (uint32_t)(iter->first)) {
       return true;
    }
  }
    return false;
}

uint32_t AscendDisassemblerHelper::GetBit(const uint8_t *opcode_data, const size_t opcode_data_len)
{
  if (opcode_data_len != 4) { // in 910B, instructions are 4 bytes
    return 0;
  }
  uint32_t data_s = (static_cast<uint32_t>(opcode_data[3]) << 24 |
      static_cast<uint32_t>(opcode_data[2]) << 16 | static_cast<uint32_t>(opcode_data[1]) << 8 |
      static_cast<uint32_t>(opcode_data[0]));
  return data_s;
}

#endif
