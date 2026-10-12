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
