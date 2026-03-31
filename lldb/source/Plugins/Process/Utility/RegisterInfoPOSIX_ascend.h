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
  // simt
  dwarf_r0_ascend = 265,
  dwarf_r1_ascend = 266,
  dwarf_r2_ascend = 267,
  dwarf_r3_ascend = 268,
  dwarf_r4_ascend = 269,
  dwarf_r5_ascend = 270,
  dwarf_r6_ascend = 271,
  dwarf_r7_ascend = 272,
  dwarf_r8_ascend = 273,
  dwarf_r9_ascend = 274,
  dwarf_r10_ascend = 275,
  dwarf_r11_ascend = 276,
  dwarf_r12_ascend = 277,
  dwarf_r13_ascend = 278,
  dwarf_r14_ascend = 279,
  dwarf_r15_ascend = 280,
  dwarf_r16_ascend = 281,
  dwarf_r17_ascend = 282,
  dwarf_r18_ascend = 283,
  dwarf_r19_ascend = 284,
  dwarf_r20_ascend = 285,
  dwarf_rx_max_id = dwarf_r20_ascend,
  // sreg
  dwarf_s0_ascend = 393,
  dwarf_s1_ascend,
  dwarf_s2_ascend,
  dwarf_s3_ascend,
  dwarf_s4_ascend,
  dwarf_s5_ascend,
  dwarf_s6_ascend,
  dwarf_s7_ascend,
  dwarf_s8_ascend,
  dwarf_s9_ascend,
  dwarf_s10_ascend,
  dwarf_s11_ascend,
  dwarf_s12_ascend,
  dwarf_s13_ascend,
  dwarf_s14_ascend,
  dwarf_s15_ascend,
  dwarf_s16_ascend,
  dwarf_s17_ascend,
  dwarf_s18_ascend,
  dwarf_s19_ascend,
  dwarf_s20_ascend,
};

// RegisterKind: EHFrame, DWARF, Generic, Process Plugin, LLDB
#define ASCEND_GPR(reg, lower_reg, kind3)                                 \
  {                                                                       \
    #reg, nullptr, 8, 0, eEncodingUint, eFormatHex,                       \
       {dwarf_##lower_reg##_ascend, dwarf_##lower_reg##_ascend,           \
        kind3, LLDB_INVALID_REGNUM, lldb_##lower_reg##_ascend},           \
       nullptr, nullptr,                                                  \
  }

#define ASCEND_GPR_4B(reg, lower_reg)                                         \
  {                                                                           \
    #reg, nullptr, 4, 0, eEncodingUint, eFormatHex,                           \
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
