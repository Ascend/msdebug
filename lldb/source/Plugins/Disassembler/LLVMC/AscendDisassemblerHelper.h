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

#ifndef liblldb_AscendDisassemblerHelper_H_
#define liblldb_AscendDisassemblerHelper_H_
#ifdef MS_DEBUGGER

#include "lldb/Utility/MessageDefines.h"
#include "lldb/Utility/ArchSpec.h"

namespace lldb_private {
class AscendDisassemblerHelper {
public:
  static void GetInstruction(const uint8_t *opcode_data,
                             const size_t opcode_data_len,
                             const lldb_private::ArchSpec &arch,
                             uint64_t &flags, uint64_t &inst_size);
};

class AscendDisassembler {
public:
  virtual void GetInstruction(const uint8_t *opcode_data,
                              const size_t opcode_data_len, uint64_t &flags,
                              const InterruptPosType pos_type,
                              uint64_t &inst_size) = 0;
};
} // namespace lldb_private

#endif
#endif // #ifndef liblldb_AscendDisassemblerHelper_H_
