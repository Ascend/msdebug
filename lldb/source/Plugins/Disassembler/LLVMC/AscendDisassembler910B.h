/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef liblldb_AscendDisassembler910B_H_
#define liblldb_AscendDisassembler910B_H_

#include "AscendDisassemblerHelper.h"

class AscendDisassembler910B : public AscendDisassembler {
public:
  void GetInstruction(const uint8_t *opcode_data, const size_t opcode_data_len,
                      uint64_t &flags, uint64_t &inst_size) override;
};

#endif // #ifndef liblldb_AscendDisassembler910B_H_
#endif
