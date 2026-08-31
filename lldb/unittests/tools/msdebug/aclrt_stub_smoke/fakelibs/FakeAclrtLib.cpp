/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */

// 单测用 fake CANN runtime 库（产物命名为 libruntime.so）。
// 供 runtime_stub 通过 dlopen/dlsym 获取 launch 路径上实际请求的 *Impl 符号，
// 全部返回成功；aclrtGetFunctionAddrImpl 返回 0 起始地址，使 GetPcStartAddr 返回 0，
// 从而让 LaunchKernelPre 在 socket 上报前提前返回，单测无需依赖 lldb-server。

#include "acl.h"

#include <cstdint>

namespace {

// aclrtGetFuncBySymbolImpl 行为开关：
// 0 = 解析成功（func 视为核函数符号，返回假 handle）；非0 = 解析失败（func 视为 funcHandle）
int g_fakeResolveMode = 0;

} // namespace

extern "C" {

void fakeSetResolveMode(int mode) { g_fakeResolveMode = mode; }

aclError aclrtSetDeviceImpl(int32_t deviceId) { return ACL_SUCCESS; }

aclError aclrtGetDeviceImpl(int32_t *deviceId) {
  if (deviceId != nullptr) {
    *deviceId = 0;
  }
  return ACL_SUCCESS;
}

aclError aclrtGetFuncBySymbolImpl(const void *symbol,
                                  aclrtFuncHandle *funcHandle) {
  if (funcHandle == nullptr) {
    return -1;
  }
  if (g_fakeResolveMode == 0 && symbol != nullptr) {
    *funcHandle = reinterpret_cast<aclrtFuncHandle>(0x1234);
    return ACL_SUCCESS;
  }
  *funcHandle = nullptr;
  return -1;
}

aclError aclrtGetFunctionNameImpl(aclrtFuncHandle funcHandle, uint32_t maxLen,
                                  char *name) {
  if (name != nullptr && maxLen > 0) {
    name[0] = '\0';
  }
  return ACL_SUCCESS;
}

// 返回空起始地址：GetRawPcStartAddr 返回 0 → GetPcStartAddr 返回 0，
// LaunchKernelPre 在 SendKernelInfo（socket）之前 return。
aclError aclrtGetFunctionAddrImpl(aclrtFuncHandle funcHandle, void **aicAddr,
                                  void **aivAddr) {
  if (aicAddr != nullptr) {
    *aicAddr = nullptr;
  }
  if (aivAddr != nullptr) {
    *aivAddr = nullptr;
  }
  return ACL_SUCCESS;
}

aclError aclrtSynchronizeStreamImpl(aclrtStream stream) { return ACL_SUCCESS; }

aclError aclrtLaunchSIMTKernelWithArgsArrayImpl(void *func, dim3 gridDim,
    dim3 blockDim, size_t dynUbufSize, aclrtStream stream,
    aclrtLaunchKernelCfg *cfg, void **args) {
  return ACL_SUCCESS;
}

aclError aclrtLaunchKernelWithArgsArrayImpl(void *func, uint32_t numBlocks,
    aclrtStream stream, aclrtLaunchKernelCfg *cfg, void **args) {
  return ACL_SUCCESS;
}

aclError aclrtLaunchSIMTKernelWithHostArgsImpl(void *func, dim3 gridDim,
    dim3 blockDim, size_t dynUbufSize, aclrtStream stream,
    aclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
    aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum) {
  return ACL_SUCCESS;
}

} // extern "C"
