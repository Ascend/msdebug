/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef REGISTERINFOPOSIX_ASCEND950_H
#define REGISTERINFOPOSIX_ASCEND950_H
#include "RegisterInfoPOSIX_ascend.h"
 
class RegisterInfoPOSIX_ascend950 : public RegisterInfoPOSIX_ascend {
public:
  RegisterInfoPOSIX_ascend950(const ArchSpec &target_arch);
  const RegisterSet* GetRegisterSet(size_t reg_set) const override;
  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) override;
  static const RegExtractor &GetRegExtractor();
  static uint64_t GetDbgAddr(uint64_t addr);

  static bool IsSimtPC(const RegisterInfo *reg_info);
 
protected:
  RegisterInfoPOSIX_ascend950(const ArchSpec &target_arch, uint32_t register_info_count,
                               uint32_t register_gpr_count, const RegisterInfo *register_info_p,
                               const std::map<std::string, DeviceRegisterInfo>& register_map);
};

// Both classes are in this file since they share the register table, defined in the .cpp.
class RegisterInfoPOSIXCore_ascend950 : public RegisterInfoPOSIX_ascend950 {
public:
  static const RegExtractor& GetRegExtractor();
 
public:
  RegisterInfoPOSIXCore_ascend950(const ArchSpec &target_arch);
 
  const RegisterSet* GetRegisterSet(size_t reg_set) const override;
 
  size_t GetRegisterSetCount() const override;
};




#endif //REGISTERINFOPOSIX_ASCEND950_H
#endif
