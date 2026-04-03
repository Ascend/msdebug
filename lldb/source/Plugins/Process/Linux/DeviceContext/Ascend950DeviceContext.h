/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
 
#ifdef MS_DEBUGGER
#ifndef ASCEND950DEVICECONTEXT_H
#define ASCEND950DEVICECONTEXT_H
 
#include "DeviceContext.h"
 
namespace lldb_private {


class Ascend950DeviceContext : public DeviceContext {
public:
  Ascend950DeviceContext(const ::pid_t pid, const uint32_t device_id);
  ~Ascend950DeviceContext() = default;
  SocType GetSocType() override { return m_soc_type; }
  Status GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) override;
  Status GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) override;
  Status CheckRegisterAddr(CoreType core_type, uint64_t addr) override;
 
  Status ReadRegister(const RegisterInfo *reg_info,
                      const InterruptPosInfo &pos_info, RegisterValue &value) override;

  MemType GetStackMemType() const override;

  Status InvalidInstrCache(const lldb::addr_t &addr,
      const InterruptPosInfo &pos_info, uint8_t redirect_ifu = 0) const override;

  Status GetDeviceInfo(DeviceInfo &device_info) override;

  Status Resume(const InterruptPosInfo &pos_info) const override;
 
  Status SingleStep(const InterruptPosInfo &pos_info) const override;
 
  Status RemoveHardwareBreakpoint(
      lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const override;
 
  Status SetHardwareBreakpoint(lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const override;

private:
  Status ReadRXReg(const RegisterInfo *reg_info, uint64_t base_addr,
                   const InterruptPosInfo &pos, RegisterValue &value);

  Status ReadSimtPC(const RegisterInfo *reg_info, uint64_t base_addr,
                    const InterruptPosInfo &pos, RegisterValue &value);

  Status ReadSXReg(const RegisterInfo *reg_info, const InterruptPosInfo &pos, RegisterValue &value);
private:
  SocType m_soc_type;
};
 
} // namespace lldb_private
 
#endif //ASCEND950DEVICECONTEXT_H
#endif // #ifdef MS_DEBUGGER
