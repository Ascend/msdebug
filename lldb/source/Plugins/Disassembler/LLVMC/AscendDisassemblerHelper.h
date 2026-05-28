/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef liblldb_AscendDisassemblerHelper_H_
#define liblldb_AscendDisassemblerHelper_H_

#include "lldb/Utility/ArchSpec.h"

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
                              uint64_t &inst_size) = 0;
};

#endif // #ifndef liblldb_AscendDisassemblerHelper_H_
#endif
