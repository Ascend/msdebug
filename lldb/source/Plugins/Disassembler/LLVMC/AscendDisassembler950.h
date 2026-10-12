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

#ifndef liblldb_AscendDisassembler950_H_
#define liblldb_AscendDisassembler950_H_

#include "AscendDisassemblerHelper.h"

namespace lldb_private {
class AscendDisassembler950 : public AscendDisassembler {
public:
  // bar.thread_block 指令编码（64bit，小端，有效位均在低 32bit）：
  // bit[29:17]=0000001010111, bit[5:2]=0111, bit[0]=0，其余位任意。
  static constexpr uint32_t kBarThreadBlockMask = 0x3FFE003D;
  static constexpr uint32_t kBarThreadBlockValue = 0x00AE001C;

  static bool IsBarThreadBlock(uint32_t encoding) {
    return (encoding & kBarThreadBlockMask) == kBarThreadBlockValue;
  }

  void GetInstruction(const uint8_t *opcode_data, const size_t opcode_data_len,
                      uint64_t &flags,
                      const InterruptPosType pos_type,
                      uint64_t &inst_size) override;
};
} // namespace lldb_private

#endif // #ifndef liblldb_AscendDisassembler910B_H_
#endif
