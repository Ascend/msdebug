#ifdef MS_DEBUGGER
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <vector>

#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend950.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/MessageDefines.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/Utility/StreamString.h"

using namespace lldb_private;
using namespace lldb;

class MockRegisterDataReader : public RegisterDataInterface {
public:
  MockRegisterDataReader() : m_next_error(false) {}

  Status ReadRegister(uint64_t addr, const RegisterInfo *reg_info,
                      uint32_t core_id, CoreType core_type,
                      RegisterValue &value) const override {
    if (m_next_error) {
      m_next_error = false;
      return Status("mock read error.");
    }
    uint32_t byte_size = reg_info->byte_size;
    std::vector<uint8_t> data(byte_size, 0);
    for (uint32_t i = 0; i < byte_size; ++i) {
      data[i] = static_cast<uint8_t>(addr & 0xFF) + i;
    }
    value.SetBytes(data.data(), byte_size, eByteOrderLittle);
    m_last_addr = addr;
    return Status();
  }

  void SetNextError(bool error) { m_next_error = error; }
  uint64_t GetLastAddr() const { return m_last_addr; }

private:
  mutable bool m_next_error;
  mutable uint64_t m_last_addr = 0;
};

class RegisterInfoPOSIX_ascend950Test : public ::testing::Test {
protected:
  void SetUp() override {
    ArchSpec arch("hiipu64");
    reg_info = std::make_unique<RegisterInfoPOSIX_ascend950>(arch);
    core_reg_info = std::make_unique<RegisterInfoPOSIXCore_ascend950>(arch);
    reader = std::make_unique<MockRegisterDataReader>();
  }

  void TearDown() override {
    reg_info.reset();
    core_reg_info.reset();
    reader.reset();
  }

  std::unique_ptr<RegisterInfoPOSIX_ascend950> reg_info;
  std::unique_ptr<RegisterInfoPOSIXCore_ascend950> core_reg_info;
  std::unique_ptr<MockRegisterDataReader> reader;
};

TEST_F(RegisterInfoPOSIX_ascend950Test,
       Constructor_InitWithHiipu64_SetsCorrectArch) {
  ArchSpec arch("hiipu64");
  EXPECT_EQ(reg_info->GetTargetArchitecture().GetArchitectureName(),
            arch.GetArchitectureName());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterSetCount_DebugVariant_Returns4) {
  EXPECT_EQ(reg_info->GetRegisterSetCount(), 4u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterSetCount_CoreVariant_Returns6) {
  EXPECT_EQ(core_reg_info->GetRegisterSetCount(), 6u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterSet_ValidIndex_ReturnsMatchingName) {
  const RegisterSet *set0 = reg_info->GetRegisterSet(0);
  ASSERT_NE(set0, nullptr);
  EXPECT_STREQ(set0->name, "SURegisters");

  const RegisterSet *set1 = reg_info->GetRegisterSet(1);
  ASSERT_NE(set1, nullptr);
  EXPECT_STREQ(set1->name, "MTERegisters");

  const RegisterSet *set2 = reg_info->GetRegisterSet(2);
  ASSERT_NE(set2, nullptr);
  EXPECT_STREQ(set2->name, "VECRegisters");

  const RegisterSet *set3 = reg_info->GetRegisterSet(3);
  ASSERT_NE(set3, nullptr);
  EXPECT_STREQ(set3->name, "L1Registers");
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterSet_OutOfBoundIndex_ReturnsNull) {
  EXPECT_EQ(reg_info->GetRegisterSet(100), nullptr);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       CoreGetRegisterSet_ValidIndex_ReturnsNonNull) {
  for (size_t i = 0; i < core_reg_info->GetRegisterSetCount(); ++i) {
    const RegisterSet *set = core_reg_info->GetRegisterSet(i);
    ASSERT_NE(set, nullptr);
  }
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       CoreGetRegisterSet_OutOfBoundIndex_ReturnsNull) {
  EXPECT_EQ(core_reg_info->GetRegisterSet(100), nullptr);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterCount_DebugVariant_IsPositive) {
  EXPECT_GT(reg_info->GetRegisterCount(), 0u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterCount_CoreVariant_GreaterThanDebug) {
  EXPECT_GT(core_reg_info->GetRegisterCount(), reg_info->GetRegisterCount());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterInfo_Default_ReturnsNonNull) {
  const RegisterInfo *info = reg_info->GetRegisterInfo();
  ASSERT_NE(info, nullptr);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, GetRegisterMap_Default_NotEmpty) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_FALSE(reg_map.empty());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterAddr_ValidRegName_ReturnsSuccess) {
  uint64_t addr = 0;
  Status error = reg_info->GetRegisterAddr(
      "PC", CoreType::AIC, InterruptPosType::SU_INTERRUPT, addr);
  EXPECT_TRUE(error.Success());
  EXPECT_NE(addr, 0ull);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterAddr_InvalidRegName_ReturnsFailure) {
  uint64_t addr = 0;
  Status error = reg_info->GetRegisterAddr(
      "nonexistent_reg", CoreType::AIC, InterruptPosType::SU_INTERRUPT, addr);
  EXPECT_TRUE(error.Fail());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterAddr_PCReg_ReturnsExpectedAddr) {
  uint64_t addr = 0;
  Status error = reg_info->GetRegisterAddr(
      "PC", CoreType::AIC, InterruptPosType::SU_INTERRUPT, addr);
  ASSERT_TRUE(error.Success());
  constexpr uint64_t SU_ID = 1UL;
  constexpr uint64_t ID_OFFSET = 52UL;
  uint64_t expected = SU_ID << ID_OFFSET | 64;
  EXPECT_EQ(addr, expected);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterAddr_GPR0Reg_ReturnsExpectedAddr) {
  uint64_t addr = 0;
  Status error = reg_info->GetRegisterAddr(
      "GPR0", CoreType::AIC, InterruptPosType::SU_INTERRUPT, addr);
  ASSERT_TRUE(error.Success());
  constexpr uint64_t SU_ID = 1UL;
  constexpr uint64_t ID_OFFSET = 52UL;
  uint64_t expected = SU_ID << ID_OFFSET | 0;
  EXPECT_EQ(addr, expected);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindPC_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("PC"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindGPR0_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("GPR0"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindSIMT_PC_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("SIMT_PC"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindV0_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("V0"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindS0_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("S0"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_FindR0_EntryExists) {
  auto reg_map = reg_info->GetRegisterMap();
  EXPECT_NE(reg_map.find("R0"), reg_map.end());
}

TEST_F(RegisterInfoPOSIX_ascend950Test, GetGPRSize_Default_Returns32) {
  EXPECT_EQ(reg_info->GetGPRSize(), 32u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, GetRegisterSet_SUIndex_Has59Registers) {
  const RegisterSet *set = reg_info->GetRegisterSet(0);
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->num_registers, 59u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetRegisterSet_MTEIndex_Has25Registers) {
  const RegisterSet *set = reg_info->GetRegisterSet(1);
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->num_registers, 25u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       ReadRegister_NullRegInfo_ReturnsFailure) {
  InterruptPosInfo pos_info;
  RegisterValue value;
  Status error = reg_info->ReadRegister(nullptr, pos_info, reader.get(), value);
  EXPECT_TRUE(error.Fail());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       ReadRegister_UnknownRegInfo_ReturnsFailure) {
  RegisterInfo unknown_reg = {};
  unknown_reg.name = "unknown_reg_xyz";
  unknown_reg.kinds[eRegisterKindLLDB] = LLDB_INVALID_REGNUM;
  InterruptPosInfo pos_info;
  RegisterValue value;
  Status error =
      reg_info->ReadRegister(&unknown_reg, pos_info, reader.get(), value);
  EXPECT_TRUE(error.Fail());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetAicErrorRegTable_CoreVariant_ReturnsSize10) {
  auto *table = core_reg_info->GetAicErrorRegTable();
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->size(), 10u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, GetAicErrorRegTable_Index0_IsEmpty) {
  auto *table = core_reg_info->GetAicErrorRegTable();
  ASSERT_NE(table, nullptr);
  EXPECT_TRUE(table->at(0).empty());
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetAicErrorRegTable_Index1SUError_HasOneErrRegMask) {
  auto *table = core_reg_info->GetAicErrorRegTable();
  ASSERT_NE(table, nullptr);
  auto &su_err = table->at(1);
  EXPECT_FALSE(su_err.empty());
  EXPECT_EQ(su_err.size(), 1u);
  EXPECT_NE(su_err[0].mask, 0u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       GetAicErrorRegTable_Index4VecError_HasTwoErrInfoRegs) {
  auto *table = core_reg_info->GetAicErrorRegTable();
  ASSERT_NE(table, nullptr);
  auto &vec_err = table->at(4);
  EXPECT_FALSE(vec_err.empty());
  EXPECT_EQ(vec_err.size(), 1u);
  EXPECT_EQ(vec_err[0].err_info_regs.size(), 2u);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_PCMask_EqualsMixMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("PC");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, MIX_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       RegisterMap_SIMT_PCMask_EqualsSimtVfMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("SIMT_PC");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, SIMT_VF_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       RegisterMap_MTE_WARNMask_EqualsMixMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("MTE_WARN");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, MIX_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       RegisterMap_L0SetValueMTEMask_EqualsAicMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("L0_SET_VALUE_MTE");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, AIC_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       RegisterMap_PadValOuttoubMask_EqualsAivMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("PAD_VAL_OUTTOUB");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, AIV_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_TPES0Mask_EqualsVfMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("TPES0");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, VF_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test, RegisterMap_V0Mask_EqualsSimdVfMask) {
  auto reg_map = reg_info->GetRegisterMap();
  auto it = reg_map.find("V0");
  ASSERT_NE(it, reg_map.end());
  EXPECT_EQ(it->second.core_type_support_mask, SIMD_VF_MASK);
}

TEST_F(RegisterInfoPOSIX_ascend950Test,
       RegisterMap_MultipleRegByteSizes_MatchExpected) {
  auto reg_map = reg_info->GetRegisterMap();
  auto pc_it = reg_map.find("PC");
  ASSERT_NE(pc_it, reg_map.end());
  EXPECT_EQ(pc_it->second.reg_info.byte_size, 8u);

  auto gpr0_it = reg_map.find("GPR0");
  ASSERT_NE(gpr0_it, reg_map.end());
  EXPECT_EQ(gpr0_it->second.reg_info.byte_size, 8u);

  auto v0_it = reg_map.find("V0");
  ASSERT_NE(v0_it, reg_map.end());
  EXPECT_EQ(v0_it->second.reg_info.byte_size, 256u);

  auto tpes0_it = reg_map.find("TPES0");
  ASSERT_NE(tpes0_it, reg_map.end());
  EXPECT_EQ(tpes0_it->second.reg_info.byte_size, 4u);

  auto s0_it = reg_map.find("S0");
  ASSERT_NE(s0_it, reg_map.end());
  EXPECT_EQ(s0_it->second.reg_info.byte_size, 2u);

  auto r0_it = reg_map.find("R0");
  ASSERT_NE(r0_it, reg_map.end());
  EXPECT_EQ(r0_it->second.reg_info.byte_size, 4u);
}

#endif
