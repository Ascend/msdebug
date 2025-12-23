/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER

#ifndef liblldb_AscendThreadLinux_H_
#define liblldb_AscendThreadLinux_H_

#include "DeviceContext/DeviceContext.h"
#include "NativeThreadLinux.h"
#include "AscendProcessLinux.h"

namespace lldb_private {
namespace process_linux {

class AscendThreadLinux : public NativeThreadLinux {
  friend class AscendProcessLinux;

public:
  AscendThreadLinux(AscendProcessLinux &process, lldb::tid_t tid);
  AscendProcessLinux& GetProcess();
  const AscendProcessLinux& GetProcess() const;
  NativeRegisterContextLinux &GetRegisterContext() override;

  Status Resume(uint32_t signo) override;
  void SetStopped(bool use_reg = true);
  Status RequestStop();
  Status SingleStep(uint32_t signo) override;
  void SetStoppedByDeviceBreakpoint(const InterruptEvent &event);
  void SetDeviceStoppedByTrace(const InterruptEvent &param);
  void SetStoppedBySignal(uint32_t signo, const siginfo_t *info = nullptr) override;
  void SetStoppedByDeviceCtrlC(const InterruptEvent &event);
private:
  std::unique_ptr<NativeRegisterContextLinux> m_device_reg_context_up;
};
} // namespace process_linux
} // namespace lldb_private

#endif // #ifndef liblldb_AscendThreadLinux_H_
#endif
