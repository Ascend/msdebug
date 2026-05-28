/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER

#include "AscendDisassemblerHelper.h"

#include "AscendDisassembler910B.h"
#include "AscendDisassembler310P.h"

#include <memory>

using namespace lldb_private;
using namespace std;

void AscendDisassemblerHelper::GetInstruction(const uint8_t *opcode_data,
                                              const size_t opcode_data_len,
                                              const ArchSpec &arch,
                                              uint64_t &flags,
                                              uint64_t &inst_size) {
  shared_ptr<AscendDisassembler> disa;
  switch (arch.GetSocType()) {
    case SocType::ASCEND310P:
      disa = make_shared<AscendDisassembler310P>();
      break;
    case SocType::ASCEND910B:
      disa = make_shared<AscendDisassembler910B>();
      break;
    default:
      return;
  }
  disa->GetInstruction(opcode_data, opcode_data_len, flags, inst_size);
}
#endif
