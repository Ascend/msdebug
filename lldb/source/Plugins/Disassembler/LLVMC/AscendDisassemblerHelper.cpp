/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifdef MS_DEBUGGER

#include "AscendDisassemblerHelper.h"

#include "AscendDisassembler310P.h"
#include "AscendDisassembler910B.h"
#include "AscendDisassembler950.h"

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
    case SocType::ASCEND950:
      disa = make_shared<AscendDisassembler950>();
      break;
    default:
      flags = 0;
      inst_size = 4;
      return;
  }
  disa->GetInstruction(opcode_data, opcode_data_len, flags, arch.GetPosType(), inst_size);
}
#endif
