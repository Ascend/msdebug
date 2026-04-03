/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER

#ifndef LLDB_SOURCE_PLUGINS_PROCESS_UTILITY_REGISTERINFOPOSIX_ASCEND_H
#define LLDB_SOURCE_PLUGINS_PROCESS_UTILITY_REGISTERINFOPOSIX_ASCEND_H

#include <map>

#include "RegisterInfoAndSetInterface.h"
#include "Plugins/Process/Linux/DeviceContext/DeviceContext.h"
#include "lldb/Utility/Flags.h"
#include "lldb/Utility/RegisterValue.h"

using namespace lldb_private;

enum DWARF_ASCEND_REGNUM {
  dwarf_x0_ascend = 64,
  dwarf_x1_ascend,
  dwarf_x2_ascend,
  dwarf_x3_ascend,
  dwarf_x4_ascend,
  dwarf_x5_ascend,
  dwarf_x6_ascend,
  dwarf_x7_ascend,
  dwarf_x8_ascend,
  dwarf_x9_ascend,
  dwarf_x10_ascend,
  dwarf_x11_ascend,
  dwarf_x12_ascend,
  dwarf_x13_ascend,
  dwarf_x14_ascend,
  dwarf_x15_ascend,
  dwarf_x16_ascend,
  dwarf_x17_ascend,
  dwarf_x18_ascend,
  dwarf_x19_ascend,
  dwarf_x20_ascend,
  dwarf_x21_ascend,
  dwarf_x22_ascend,
  dwarf_x23_ascend,
  dwarf_x24_ascend,
  dwarf_x25_ascend,
  dwarf_x26_ascend,
  dwarf_x27_ascend,
  dwarf_x28_ascend,
  dwarf_x29_ascend,
  dwarf_x30_ascend,
  dwarf_x31_ascend,
  dwarf_pc_ascend = 128,

  // sreg
  dwarf_s0_ascend = 137, dwarf_s1_ascend, dwarf_s2_ascend, dwarf_s3_ascend,
  dwarf_s4_ascend, dwarf_s5_ascend, dwarf_s6_ascend, dwarf_s7_ascend,
  dwarf_s8_ascend, dwarf_s9_ascend, dwarf_s10_ascend, dwarf_s11_ascend,
  dwarf_s12_ascend, dwarf_s13_ascend, dwarf_s14_ascend, dwarf_s15_ascend,
  dwarf_s16_ascend, dwarf_s17_ascend, dwarf_s18_ascend, dwarf_s19_ascend,
  dwarf_s20_ascend, dwarf_s21_ascend, dwarf_s22_ascend, dwarf_s23_ascend,
  dwarf_s24_ascend, dwarf_s25_ascend, dwarf_s26_ascend, dwarf_s27_ascend,
  dwarf_s28_ascend, dwarf_s29_ascend, dwarf_s30_ascend, dwarf_s31_ascend,
  dwarf_s32_ascend, dwarf_s33_ascend, dwarf_s34_ascend, dwarf_s35_ascend,
  dwarf_s36_ascend, dwarf_s37_ascend, dwarf_s38_ascend, dwarf_s39_ascend,
  dwarf_s40_ascend, dwarf_s41_ascend, dwarf_s42_ascend, dwarf_s43_ascend,
  dwarf_s44_ascend, dwarf_s45_ascend, dwarf_s46_ascend, dwarf_s47_ascend,
  dwarf_s48_ascend, dwarf_s49_ascend, dwarf_s50_ascend, dwarf_s51_ascend,
  dwarf_s52_ascend, dwarf_s53_ascend, dwarf_s54_ascend, dwarf_s55_ascend,
  dwarf_s56_ascend, dwarf_s57_ascend, dwarf_s58_ascend, dwarf_s59_ascend,
  dwarf_s60_ascend, dwarf_s61_ascend, dwarf_s62_ascend, dwarf_s63_ascend,

  dwarf_s64_ascend = 201, dwarf_s65_ascend, dwarf_s66_ascend, dwarf_s67_ascend,
  dwarf_s68_ascend, dwarf_s69_ascend, dwarf_s70_ascend, dwarf_s71_ascend,
  dwarf_s72_ascend, dwarf_s73_ascend, dwarf_s74_ascend, dwarf_s75_ascend,
  dwarf_s76_ascend, dwarf_s77_ascend, dwarf_s78_ascend, dwarf_s79_ascend,
  dwarf_s80_ascend, dwarf_s81_ascend, dwarf_s82_ascend, dwarf_s83_ascend,
  dwarf_s84_ascend, dwarf_s85_ascend, dwarf_s86_ascend, dwarf_s87_ascend,
  dwarf_s88_ascend, dwarf_s89_ascend, dwarf_s90_ascend, dwarf_s91_ascend,
  dwarf_s92_ascend, dwarf_s93_ascend, dwarf_s94_ascend, dwarf_s95_ascend,
  // simt
  dwarf_r0_ascend = 265, dwarf_r1_ascend, dwarf_r2_ascend, dwarf_r3_ascend,
  dwarf_r4_ascend, dwarf_r5_ascend, dwarf_r6_ascend, dwarf_r7_ascend,
  dwarf_r8_ascend, dwarf_r9_ascend, dwarf_r10_ascend, dwarf_r11_ascend,
  dwarf_r12_ascend, dwarf_r13_ascend, dwarf_r14_ascend, dwarf_r15_ascend,
  dwarf_r16_ascend, dwarf_r17_ascend, dwarf_r18_ascend, dwarf_r19_ascend,
  dwarf_r20_ascend, dwarf_r21_ascend, dwarf_r22_ascend, dwarf_r23_ascend,
  dwarf_r24_ascend, dwarf_r25_ascend, dwarf_r26_ascend, dwarf_r27_ascend,
  dwarf_r28_ascend, dwarf_r29_ascend, dwarf_r30_ascend, dwarf_r31_ascend,
  dwarf_r32_ascend, dwarf_r33_ascend, dwarf_r34_ascend, dwarf_r35_ascend,
  dwarf_r36_ascend, dwarf_r37_ascend, dwarf_r38_ascend, dwarf_r39_ascend,
  dwarf_r40_ascend, dwarf_r41_ascend, dwarf_r42_ascend, dwarf_r43_ascend,
  dwarf_r44_ascend, dwarf_r45_ascend, dwarf_r46_ascend, dwarf_r47_ascend,
  dwarf_r48_ascend, dwarf_r49_ascend, dwarf_r50_ascend, dwarf_r51_ascend,
  dwarf_r52_ascend, dwarf_r53_ascend, dwarf_r54_ascend, dwarf_r55_ascend,
  dwarf_r56_ascend, dwarf_r57_ascend, dwarf_r58_ascend, dwarf_r59_ascend,
  dwarf_r60_ascend, dwarf_r61_ascend, dwarf_r62_ascend, dwarf_r63_ascend,
  dwarf_r64_ascend, dwarf_r65_ascend, dwarf_r66_ascend, dwarf_r67_ascend,
  dwarf_r68_ascend, dwarf_r69_ascend, dwarf_r70_ascend, dwarf_r71_ascend,
  dwarf_r72_ascend, dwarf_r73_ascend, dwarf_r74_ascend, dwarf_r75_ascend,
  dwarf_r76_ascend, dwarf_r77_ascend, dwarf_r78_ascend, dwarf_r79_ascend,
  dwarf_r80_ascend, dwarf_r81_ascend, dwarf_r82_ascend, dwarf_r83_ascend,
  dwarf_r84_ascend, dwarf_r85_ascend, dwarf_r86_ascend, dwarf_r87_ascend,
  dwarf_r88_ascend, dwarf_r89_ascend, dwarf_r90_ascend, dwarf_r91_ascend,
  dwarf_r92_ascend, dwarf_r93_ascend, dwarf_r94_ascend, dwarf_r95_ascend,
  dwarf_r96_ascend, dwarf_r97_ascend, dwarf_r98_ascend, dwarf_r99_ascend,
  dwarf_r100_ascend, dwarf_r101_ascend, dwarf_r102_ascend, dwarf_r103_ascend,
  dwarf_r104_ascend, dwarf_r105_ascend, dwarf_r106_ascend, dwarf_r107_ascend,
  dwarf_r108_ascend, dwarf_r109_ascend, dwarf_r110_ascend, dwarf_r111_ascend,
  dwarf_r112_ascend, dwarf_r113_ascend, dwarf_r114_ascend, dwarf_r115_ascend,
  dwarf_r116_ascend, dwarf_r117_ascend, dwarf_r118_ascend, dwarf_r119_ascend,
  dwarf_r120_ascend, dwarf_r121_ascend, dwarf_r122_ascend, dwarf_r123_ascend,
  dwarf_r124_ascend, dwarf_r125_ascend, dwarf_r126_ascend,
  dwarf_rx_max_id = dwarf_r126_ascend,
  // sreg
  dwarf_tpes0_ascend = 457, dwarf_tpes1_ascend, dwarf_tpes2_ascend, dwarf_tpes3_ascend,
  dwarf_tpes4_ascend, dwarf_tpes5_ascend, dwarf_tpes6_ascend, dwarf_tpes7_ascend,
  dwarf_tpes8_ascend, dwarf_tpes9_ascend, dwarf_tpes10_ascend, dwarf_tpes11_ascend,
  dwarf_tpes12_ascend, dwarf_tpes13_ascend, dwarf_tpes14_ascend, dwarf_tpes15_ascend,
  dwarf_tpes16_ascend, dwarf_tpes17_ascend, dwarf_tpes18_ascend, dwarf_tpes19_ascend,
  dwarf_tpes20_ascend, dwarf_tpes21_ascend, dwarf_tpes22_ascend, dwarf_tpes23_ascend,
  dwarf_tpes24_ascend, dwarf_tpes25_ascend, dwarf_tpes26_ascend, dwarf_tpes27_ascend,
  dwarf_tpes28_ascend, dwarf_tpes29_ascend, dwarf_tpes30_ascend, dwarf_tpes31_ascend,

  dwarf_tpesl0_ascend = 489, dwarf_tpesl1_ascend, dwarf_tpesl2_ascend, dwarf_tpesl3_ascend,
  dwarf_tpesl4_ascend, dwarf_tpesl5_ascend, dwarf_tpesl6_ascend, dwarf_tpesl7_ascend,
  dwarf_tpesl8_ascend, dwarf_tpesl9_ascend, dwarf_tpesl10_ascend, dwarf_tpesl11_ascend,
  dwarf_tpesl12_ascend, dwarf_tpesl13_ascend, dwarf_tpesl14_ascend, dwarf_tpesl15_ascend,

  dwarf_tpesll0_ascend = 505, dwarf_tpesll1_ascend, dwarf_tpesll2_ascend, dwarf_tpesll3_ascend,
  dwarf_tpesll4_ascend, dwarf_tpesll5_ascend, dwarf_tpesll6_ascend, dwarf_tpesll7_ascend,

  dwarf_sx_max_id = dwarf_tpesll7_ascend,
};

// RegisterKind: EHFrame, DWARF, Generic, Process Plugin, LLDB
#define ASCEND_GPR(reg, lower_reg, kind3)                                 \
  {                                                                       \
    #reg, nullptr, 8, 0, eEncodingUint, eFormatHex,                       \
       {dwarf_##lower_reg##_ascend, dwarf_##lower_reg##_ascend,           \
        kind3, LLDB_INVALID_REGNUM, lldb_##lower_reg##_ascend},           \
       nullptr, nullptr,                                                  \
  }

#define ASCEND_GPR_NBYTES(reg, lower_reg, byte_size)                                         \
  {                                                                           \
    #reg, nullptr, byte_size, 0, eEncodingUint, eFormatHex,                           \
       {dwarf_##lower_reg##_ascend, dwarf_##lower_reg##_ascend,               \
        LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, lldb_##lower_reg##_ascend}, \
       nullptr, nullptr,                                                      \
  }

#define ASCEND_GPR_ALT(reg, lower_reg, alt, kind3)                        \
  {                                                                       \
    #reg, #alt, 8, 0, eEncodingUint, eFormatHex,                          \
       {dwarf_##lower_reg##_ascend, dwarf_##lower_reg##_ascend,           \
        kind3, LLDB_INVALID_REGNUM, lldb_##lower_reg##_ascend},           \
       nullptr, nullptr,                                                  \
  }

#define ASCEND_REG(reg, kind5)                                            \
  {                                                                       \
    #reg, nullptr, 8, 0, eEncodingUint, eFormatHex,                       \
       {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,                         \
        LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, kind5},                 \
       nullptr, nullptr,                                                  \
  }

#define ASCEND_REG_4B(reg, kind5)                                            \
  {                                                                       \
    #reg, nullptr, 4, 0, eEncodingUint, eFormatHex,                       \
       {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,                         \
        LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, kind5},                 \
       nullptr, nullptr,                                                  \
  }

#define ASCEND_REG_16B(reg, kind5)                                         \
  {                                                                       \
    #reg, nullptr, 16, 0, eEncodingUint, eFormatHex,                      \
       {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,                         \
        LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, kind5},                 \
       nullptr, nullptr,                                                  \
  }
 
#define ASCEND_REG_32B(reg, kind5)                                         \
  {                                                                       \
    #reg, nullptr, 32, 0, eEncodingVector, eFormatHex,                      \
       {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,                         \
        LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, kind5},                 \
       nullptr, nullptr,                                                  \
  }

struct DeviceRegisterInfo {
  RegisterInfo reg_info;
  uint64_t addr;
  uint8_t core_type_support_mask;
};

struct DeviceRegisterTable {
  RegisterInfo *register_info_p;
  uint32_t num_register;
  uint32_t num_gpr_register;
};

// Record the xx_err_info register id and the pc replacement bit.
struct ErrInfoReg {
  uint32_t reg_num;
  // pair.first: the valid bits of error info register
  // pair.second: the valid bits to replace original pc register
  std::vector<std::pair<uint32_t, uint32_t>> pc_mask_map;
};

// Find xx_err_info register by aic_error_reg mask bit
struct ErrRegMask {
  uint32_t mask;
  std::vector<ErrInfoReg> err_info_regs;
};

struct RegExtractor {
  RegExtractor(const DeviceRegisterInfo *table, size_t tableSize) {
    // 寄存器信息改成可变的，为了offset赋值
    raw_register_infos.resize(tableSize);
    std::transform(table, table + tableSize, raw_register_infos.begin(),
                   [](const DeviceRegisterInfo &info) {return RegisterInfo(info.reg_info);} );
    register_map.clear();
    uint32_t offset = 0;
    for (size_t i = 0; i < tableSize; i++) {
      const auto &info = table[i];
      register_map[info.reg_info.name] = info;
      register_map[info.reg_info.name].reg_info.byte_offset = offset;
      raw_register_infos[i].byte_offset = offset;
      offset += info.reg_info.byte_size;
    }
  }
  std::vector<RegisterInfo> raw_register_infos;
  std::map<std::string, DeviceRegisterInfo> register_map;
};

class RegisterInfoPOSIX_ascend: public RegisterInfoInterface {
public:
  RegisterInfoPOSIX_ascend(const ArchSpec &target_arch, uint32_t register_info_count,
  uint32_t register_gpr_count, const RegisterInfo *register_info_p,
  const std::map<std::string, DeviceRegisterInfo>& register_map);

  size_t GetGPRSize() const override;

  const RegisterInfo *GetRegisterInfo() const override;

  uint32_t GetRegisterCount() const override;

  virtual const RegisterSet* GetRegisterSet(size_t reg_set) const;

  virtual size_t GetRegisterSetCount() const;

  virtual std::map<std::string, DeviceRegisterInfo> GetRegisterMap();

  virtual Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr);

  virtual const std::vector<std::vector<ErrRegMask>> *GetAicErrorRegTable() { return nullptr; }

protected:
  uint32_t m_register_info_count;
  uint32_t m_register_gpr_count;
  const RegisterInfo *m_register_info_p;
  std::map<std::string, DeviceRegisterInfo> m_register_map;
};

#endif // ifndef LLDB_SOURCE_PLUGINS_PROCESS_UTILITY_REGISTERINFOPOSIX_ASCEND_H
#endif // ifdef MS_DEBUGGER
