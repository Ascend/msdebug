/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef RUNTIME_STUB_H
#define RUNTIME_STUB_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "rt_stub_log.h"
#include "get_tiling_key.h"
#include "Singleton.h"

#define VISIBILITY_EXPORT __attribute__((visibility("default")))

struct StubFuncInfo {
    std::string name;
    MSDEBUG_ERROR_CODE errorCode;
    void *funcPtr;
};

struct rtDevBinary_t {
    uint32_t magic;    // magic number
    uint32_t version;  // version of binary
    const void *data;  // binary data
    uint64_t length;   // binary length
};

class MapManager : public Singleton<MapManager, false> {
public:
  const KernelInfo &GetKernelInfo(const void *handle) const
  {
    if (handleKernelInfoMap_.find(handle) != handleKernelInfoMap_.end()) {
      return handleKernelInfoMap_.at(handle);
    }
    ThrowErrorCode(GET_KERNEL_INFO_ERROR);
    static KernelInfo empty;
    return empty;
  }

  const void* GetHandle(const void *stubFunc) const
  {
    if (funcHandleMap_.find(stubFunc) != funcHandleMap_.end()) {
      return funcHandleMap_.at(stubFunc);
    }
    return nullptr;
  }

  void AddKernelInfoMap(const void *handle, const KernelInfo& kernelInfo)
  {
    if (handleKernelInfoMap_.find(handle) == handleKernelInfoMap_.end()) {
      handleKernelInfoMap_[handle] = kernelInfo;
    }
  }

  void AddFuncHandleMap(const void *func, const void *handle)
  {
    if (funcHandleMap_.find(func) == funcHandleMap_.end()) {
      funcHandleMap_[func] = handle;
    }
  }

private:
  std::unordered_map<const void *, KernelInfo> handleKernelInfoMap_;
  std::unordered_map<const void *, const void *> funcHandleMap_;
};


int32_t SendInfoAndWaitForReply(const std::string &buf);
int32_t SendStreamId(uint32_t stream_id);
int32_t SendDeviceInfo(int32_t device, const std::string &socVersion, pid_t tgid);
int32_t SendKernelInfo(const std::string &kernelName, const std::string &kernelHash,
                       const std::vector<char> &elf, uint64_t pcAddr);
void ShowKernelLaunchInfo(const std::string &kernelName, int32_t deviceId);
void EnvCheck();
bool BinaryRegisterPost(const rtDevBinary_t *bin, void *hdl, const std::string &hash, std::string &err);
std::string GetKernelNameByTilingKey(const void *hdl, uint64_t tilingKey);
uint64_t GetFixedPcStartAddr(const KernelInfo &kernelInfo,
    const std::string &targetKernelName, uint64_t pcStartAddr);
int32_t ConvertToVisibleDeviceId(int32_t devId);

extern "C" {
using rtError_t = int32_t;
using rtSmDesc_t = void;
using rtStream_t = void *;
using rtArgsEx_t = void;

struct rtTaskCfgInfo_t {
    uint8_t qos;
    uint8_t partId;
    uint8_t schemMode; // rtschemModeType_t 0:normal;1:batch;2:sync
    uint8_t res[1];
};

VISIBILITY_EXPORT rtError_t rtSetDevice(int32_t device);
VISIBILITY_EXPORT rtError_t rtSetDeviceEx(int32_t device);
VISIBILITY_EXPORT rtError_t rtGetDevice(int32_t *device);
VISIBILITY_EXPORT rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device);
VISIBILITY_EXPORT rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc,
    const char *stubName, const void *devFunc, uint32_t funcMode);
VISIBILITY_EXPORT rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim,
    void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream);
VISIBILITY_EXPORT rtError_t rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo);
VISIBILITY_EXPORT rtError_t rtKernelLaunchWithHandle(void *hdl, const uint64_t tilingKey, uint32_t blockDim,
    rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm, const void* kernelInfo);
VISIBILITY_EXPORT rtError_t rtKernelLaunchWithHandleV2(void *hdl, const uint64_t tilingKey,
    uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm,
    const rtTaskCfgInfo_t *cfgInfo);
VISIBILITY_EXPORT rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **hdl);
VISIBILITY_EXPORT rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void **handle);
VISIBILITY_EXPORT rtError_t rtGetTaskIdAndStreamID(uint32_t *taskId, uint32_t *streamId);
VISIBILITY_EXPORT rtError_t rtStreamSynchronizeWithTimeout(rtStream_t stream, int32_t timeout);
VISIBILITY_EXPORT __attribute__((noinline)) void MSBreakOnLaunch();
}

#endif
#endif
