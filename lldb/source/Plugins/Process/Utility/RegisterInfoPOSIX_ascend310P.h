/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef REGISTERINFOPOSIX_ASCEND310P_H
#define REGISTERINFOPOSIX_ASCEND310P_H
#include "RegisterInfoPOSIX_ascend.h"

class RegisterInfoPOSIX_ascend310P : public RegisterInfoPOSIX_ascend {
public:

  RegisterInfoPOSIX_ascend310P(const ArchSpec &target_arch);

  const RegisterSet *GetRegisterSet(size_t reg_set) const override;

  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) override;

  static const RegExtractor& GetRegExtractor();
};

#endif //REGISTERINFOPOSIX_ASCEND310P_H
#endif
