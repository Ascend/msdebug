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

  Status CheckRegisterAddr(CoreType core_type, uint64_t addr) const override;

  MemType GetStackMemType() const override;

  bool IsValidStack(lldb::addr_t addr, size_t size) override;

  Status InvalidInstrCache(const lldb::addr_t &addr,
      const InterruptPosInfo &pos_info, uint8_t redirect_ifu = 0) const override;

  Status GetDeviceInfo(DeviceInfo &device_info) override;

  Status Resume(const InterruptPosInfo &pos_info) const override;

  Status SingleStep(const InterruptPosInfo &pos_info) const override;

  Status RemoveHardwareBreakpoint(
      lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const override;

  Status SetHardwareBreakpoint(lldb::addr_t addr, uint16_t stream_id, const InterruptPosInfo &pos_info) const override;

  Status GetWarpsInfo(std::vector<WarpInfo> &warps_info, const InterruptPosInfo &m_pos_info) const override;
  Status GetWarpInfo(WarpInfo &info, uint16_t warp_id, const InterruptPosInfo &m_pos_info) const override;

private:
  SocType m_soc_type;
};

class Ascend950DTDeviceContext : public Ascend950DeviceContext {
public:
  Ascend950DTDeviceContext(const ::pid_t pid, const uint32_t device_id): Ascend950DeviceContext(pid, device_id)
  {
    m_soc_type = SocType::ASCEND950DT;
    m_aclrt_handle = nullptr;
  }
  ~Ascend950DTDeviceContext() = default;

  virtual Status Init() override;
  size_t ReadGlobalMemory(lldb::addr_t addr, size_t size, void *data) override;
  size_t WriteGlobalMemory(lldb::addr_t addr, size_t size, const void *data) override;
  virtual void AddIpcMemInfo(lldb::addr_t addr, size_t size, std::vector<char> key) override;
  virtual void RemoveIpcMemInfo(lldb::addr_t addr) override;

private:
  SocType m_soc_type;
  void *m_aclrt_handle;
};


} // namespace lldb_private

#endif //ASCEND950DEVICECONTEXT_H
#endif // #ifdef MS_DEBUGGER
