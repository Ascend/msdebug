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
#ifndef liblldb_ASCEND910B_DEVICE_CONTEXT_H_
#define liblldb_ASCEND910B_DEVICE_CONTEXT_H_

#include "DeviceContext.h"

namespace lldb_private {

class Ascend910BDeviceContext : public DeviceContext {
public:
  Ascend910BDeviceContext(const ::pid_t pid, const uint32_t device_id, const uint32_t virtual_device_id);
  ~Ascend910BDeviceContext() = default;
  SocType GetSocType() override { return m_soc_type; }
  Status CheckRegisterAddr(CoreType core_type, uint64_t addr) const override;

  MemType GetStackMemType() const override;

  size_t ReadLocalMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                 const InterruptPosInfo &pos_info, void *data) override;
private:
  SocType m_soc_type;
};

} // namespace lldb_private

#endif // #ifndef liblldb_ASCEND910B_DEVICE_CONTEXT_H_
#endif // #ifdef MS_DEBUGGER
