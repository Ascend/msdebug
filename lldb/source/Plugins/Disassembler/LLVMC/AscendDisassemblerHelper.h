/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2026. All rights reserved.
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
