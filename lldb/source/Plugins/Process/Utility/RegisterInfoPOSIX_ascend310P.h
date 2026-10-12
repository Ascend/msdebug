/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifdef MS_DEBUGGER
#ifndef REGISTERINFOPOSIX_ASCEND310P_H
#define REGISTERINFOPOSIX_ASCEND310P_H
#include "RegisterInfoPOSIX_ascend.h"

class RegisterInfoPOSIX_ascend310P : public RegisterInfoPOSIX_ascend {
public:

  RegisterInfoPOSIX_ascend310P(const ArchSpec &target_arch);

  const RegisterSet *GetRegisterSet(size_t reg_set) const override;

  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type,
                         InterruptPosType pos_type, uint64_t &addr) override;

  static const RegExtractor& GetRegExtractor();
};

#endif //REGISTERINFOPOSIX_ASCEND310P_H
#endif
