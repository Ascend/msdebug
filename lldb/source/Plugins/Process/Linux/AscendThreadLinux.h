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
  void SetStopThreadIdx(uint16_t x, uint16_t y, uint16_t z);
  void SetStoppedByDeviceBreakpoint(const InterruptEvent &event);
  void SetDeviceStoppedByTrace(const InterruptEvent &param);
  void SetStoppedBySignal(uint32_t signo, const siginfo_t *info = nullptr) override;
  void SetStoppedByDeviceCtrlC(const InterruptEvent &event);
  void SetStoppedByInternalBreakpoint();
  void SetStopInfoDetails(ThreadStopInfo &stop_info, const InterruptEvent &event);

private:
  std::unique_ptr<NativeRegisterContextLinux> m_device_reg_context_up;
};
} // namespace process_linux
} // namespace lldb_private

#endif // #ifndef liblldb_AscendThreadLinux_H_
#endif
