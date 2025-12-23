/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef REGISTERINFOPOSIX_ASCEND910B_H
#define REGISTERINFOPOSIX_ASCEND910B_H
#include "RegisterInfoPOSIX_ascend.h"


class RegisterInfoPOSIX_ascend910B : public RegisterInfoPOSIX_ascend {
public:
  static const RegExtractor& GetRegExtractor();

public:
  RegisterInfoPOSIX_ascend910B(const ArchSpec &target_arch);

protected:
  RegisterInfoPOSIX_ascend910B(const ArchSpec &target_arch, uint32_t register_info_count,
                               uint32_t register_gpr_count, const RegisterInfo *register_info_p,
                               const std::map<std::string, DeviceRegisterInfo>& register_map);

  const RegisterSet* GetRegisterSet(size_t reg_set) const override;
  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) override;
};

// Both classes are in this file since they share the register table, defined in the .cpp.
class RegisterInfoPOSIXCore_ascend910B : public RegisterInfoPOSIX_ascend910B {
public:
  static const RegExtractor& GetRegExtractor();

public:
  RegisterInfoPOSIXCore_ascend910B(const ArchSpec &target_arch);

  const RegisterSet* GetRegisterSet(size_t reg_set) const override;

  size_t GetRegisterSetCount() const override;

  const std::vector<std::vector<ErrRegMask>> *GetAicErrorRegTable() override;
};

#endif //REGISTERINFOPOSIX_ASCEND910B_H
#endif
