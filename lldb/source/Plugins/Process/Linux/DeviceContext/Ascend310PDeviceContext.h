/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef liblldb_ASCEND310P_DEVICE_CONTEXT_H_
#define liblldb_ASCEND310P_DEVICE_CONTEXT_H_

#include "DeviceContext.h"

namespace lldb_private {

class Ascend310PDeviceContext : public DeviceContext {
public:
  Ascend310PDeviceContext(const ::pid_t pid, const uint32_t device_id);
  ~Ascend310PDeviceContext() = default;
  SocType GetSocType() override { return m_soc_type; }
  Status CheckRegisterAddr(CoreType core_type, uint64_t addr) const override;

  MemType GetStackMemType() const override;

  Status Resume(const InterruptPosInfo &pos_info) const override;

  Status InvalidInstrCache(const lldb::addr_t &addr, const InterruptPosInfo &pos_info,
                           uint8_t redirect_ifu = 0) const override;


  size_t ReadLocalMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                const InterruptPosInfo &pos_info, void *data) override;
private:
  size_t ReadL0CMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                        const InterruptPosInfo &pos_info, void *data);

  size_t ReadL0ABMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                        const InterruptPosInfo &pos_info, void *data);
private:
  SocType m_soc_type;
};

} // namespace lldb_private

#endif // #ifndef liblldb_ASCEND910B_DEVICE_CONTEXT_H_
#endif // #ifdef MS_DEBUGGER
