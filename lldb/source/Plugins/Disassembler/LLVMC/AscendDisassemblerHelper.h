/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef liblldb_AscendDisassemblerHelper_H_
#define liblldb_AscendDisassemblerHelper_H_

#include <map>
#include <memory>

enum class InstValue : uint32_t {
    JUMP = 0x40000000,
    JUMPC =  0x40200000,
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

class AscendDisassemblerHelper {
public:
    static void GetInstruction(const uint8_t* opcode_data,
      const size_t opcode_data_len, uint64_t &flags, uint64_t &inst_size);
private:
    static uint32_t GetBit(const uint8_t *opcode_data, const size_t opcode_data_len);
    static bool IsTargetInst(const uint8_t *opcode_data, const size_t opcode_data_len,
      const std::map<InstValue, InstMode> &tar_map);
};

#endif // #ifndef liblldb_AscendDisassemblerHelper_H_
#endif
