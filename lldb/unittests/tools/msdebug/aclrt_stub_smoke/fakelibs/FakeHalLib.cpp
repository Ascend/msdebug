/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */

// 单测用 fake libascend_hal.so。OpenDrvLib 仅要求 dlopen 成功；
// drvDeviceGetBareTgid / halMemAdvise 在 launch happy path 不会被实际调用，给出平凡实现。

#include "acl.h"

#include <unistd.h>

extern "C" {

pid_t drvDeviceGetBareTgid(void) { return getpid(); }

drvError_t halMemAdvise(DVdeviceptr ptr, size_t count, unsigned int advise,
                        DVdevice device) {
  return 0;
}

} // extern "C"
