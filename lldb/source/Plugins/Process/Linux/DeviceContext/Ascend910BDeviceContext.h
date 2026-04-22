/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef liblldb_ASCEND910B_DEVICE_CONTEXT_H_
#define liblldb_ASCEND910B_DEVICE_CONTEXT_H_

#include "DeviceContext.h"

namespace lldb_private {

class Ascend910BDeviceContext : public DeviceContext {
public:
  Ascend910BDeviceContext(const ::pid_t pid, const uint32_t device_id);
  ~Ascend910BDeviceContext() = default;
  SocType GetSocType() override { return m_soc_type; }
  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) override;
  Status GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) override;
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
