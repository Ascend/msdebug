/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef liblldb_AscendDisassembler310P_H_
#define liblldb_AscendDisassembler310P_H_

#include "AscendDisassemblerHelper.h"

namespace lldb_private {
class AscendDisassembler310P : public AscendDisassembler {
public:
  void GetInstruction(const uint8_t* opcode_data,
      const size_t opcode_data_len, uint64_t &flags,
      const InterruptPosType pos_type, uint64_t &inst_size) override;
};
} // namespace lldb_private

#endif // #ifndef liblldb_AscendDisassembler310P_H_
#endif
