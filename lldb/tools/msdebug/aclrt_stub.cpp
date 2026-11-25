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

#include "HijackedLayerManager.h"
#include "StubSymbolResolver.h"
#include "acl.h"
#include "runtime_stub.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <string>
#include <unistd.h>
#include <unordered_set>

using namespace std;

// 记录halMemAdvise设置的内存属性advise值，key为(ptr, device)
// 用于在调用halMemAdvise前判断advise是否已是预期值，避免重复调用
// 定义在匿名命名空间外，供runtime_stub.cpp共享使用
std::map<std::pair<uint64_t, uint32_t>, unsigned int> &GetHalMemAdviseMap() {
  static std::map<std::pair<uint64_t, uint32_t>, unsigned int> inst{};
  return inst;
}

// 共享函数声明（定义在匿名命名空间后），供匿名命名空间内调用
MemAdviseRestoreInfo &GetMemAdviseRestoreInfo();
drvError_t halMemAdviseOrigin(DVdeviceptr ptr, size_t count,
                              unsigned int advise, DVdevice device);
void SetMemAdviseIfNecessary(uint64_t base_ptr, size_t psize, int32_t deviceId);
void RestoreMemAdvise();

namespace {

typedef pid_t (*drvDeviceGetBareTgidFunc)(void);

void *GetStubFuncPtr(const std::string funcName, bool throw_error = true);

static std::map<const void *, std::string> &GetStubFuncPtrNameMap()
{
    static std::map<const void *, std::string> inst{};
    return inst;
}

static std::map<const aclrtBinary, rtDevBinary_t> &GetDevBinaryMap()
{
    static std::map<const aclrtBinary, rtDevBinary_t> inst{};
    return inst;
}

aclError aclrtGetDeviceImpl(int32_t *deviceId)
{
    using FuncType = decltype(&aclrtGetDeviceImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(deviceId);
    if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
        ThrowErrorCode(ACLRT_GET_DEVICE_IMPL_FAILED_ERR);
    }
    return ret;
}

aclError aclrtStreamGetIdImpl(aclrtStream stream, int32_t *streamId)
{
    using FuncType = decltype(&aclrtStreamGetIdImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(stream, streamId);
    if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
        PrintErrorCode(ACLRT_STREAM_GET_ID_IMPL_FAILED_ERR);
        return ret;
    }
    return ret;
}

aclError aclrtSynchronizeStreamImpl(aclrtStream stream)
{
    using FuncType = decltype(&aclrtSynchronizeStreamImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(stream);
    if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
        return ret;
    }
    return ret;
}

aclError aclrtGetLogicDevIdByUserDevIdImpl(const int32_t userDevid,
                                           int32_t *const logicDevId) {
  using FuncType = decltype(&aclrtGetLogicDevIdByUserDevIdImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  auto ret = func(userDevid, logicDevId);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }
  return ret;
}

std::string GetKernelNameFromStubFunc(const void *stubFunc)
{
    const auto &stubFuncNameMap = GetStubFuncPtrNameMap();
    auto it = stubFuncNameMap.find(stubFunc);
    if (it != stubFuncNameMap.end()) {
      return it->second;
    }
    RT_STUB_LOG_INFO(
        "stubFunc is not found in map, try aclrtGetFunctionNameImpl\n");
    using FuncType = aclError (*)(aclrtFuncHandle, uint32_t, char *);
    auto func = (FuncType)GetStubFuncPtr("aclrtGetFunctionNameImpl", false);
    if (func != nullptr) {
      std::string name(4096, '\0');
      if (func(const_cast<aclrtFuncHandle>(stubFunc),
               static_cast<uint32_t>(name.size()),
               name.data()) == ACL_SUCCESS) {
        std::string kernelName(name.c_str());
        if (!kernelName.empty()) {
          return kernelName;
        }
      }
    }
    return "anonymous";
}

void *GetStubFuncPtr(const std::string funcName, bool throw_error)
{
  return StubSymbolResolver::Instance().Resolve(funcName, throw_error);
}

const char *aclrtGetSocNameImpl()
{
    using FuncType = decltype(&aclrtGetSocNameImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    return func();
}

aclrtBinHandle GetBinHandleByFuncHandle(aclrtFuncHandle funcHandle) {
  using FuncType = aclError (*)(aclrtFuncHandle, aclrtBinHandle *);
  auto func = (FuncType)GetStubFuncPtr("aclrtFunctionGetBinaryImpl", false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Get aclrtFunctionGetBinaryImpl stub failed\n");
    return nullptr;
  }
  aclrtBinHandle binHandle = nullptr;
  auto ret = func(funcHandle, &binHandle);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("aclrtFunctionGetBinaryImpl failed. ret=%d\n", ret);
    return nullptr;
  }
  return binHandle;
}

// 调用aclrtGetFunctionAddrImpl获取核函数的原始起始地址(aic或aiv)
uint64_t GetRawPcStartAddr(aclrtFuncHandle funcHandle) {
  using FuncType = decltype(&aclrtGetFunctionAddrImpl);
  auto func = (FuncType)GetStubFuncPtr("aclrtGetFunctionAddrImpl");
  void *aicAddr{nullptr};
  void *aivAddr{nullptr};
  auto ret = func(funcHandle, &aicAddr, &aivAddr);
  if (ret != 0) {
    RT_STUB_LOG_ERROR("aclrtGetFunctionAddrImpl failed. ret=%d\n", ret);
    PrintErrorCode(ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR);
    return 0;
  }
  if (aicAddr) {
    return reinterpret_cast<uint64_t>(aicAddr);
  }
  if (aivAddr) {
    return reinterpret_cast<uint64_t>(aivAddr);
  }
  RT_STUB_LOG_ERROR("aclrtGetFunctionAddrImpl get addr all zero\n");
  PrintErrorCode(ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR);
  return 0;
}

// 根据kernel信息修正pc起始地址
uint64_t FixPcStartAddr(aclrtFuncHandle funcHandle, uint64_t pcStartAddr) {
  std::string targetKernelName = GetKernelNameFromStubFunc(funcHandle);
  auto binHandle = GetBinHandleByFuncHandle(funcHandle);
  if (binHandle == nullptr) {
    RT_STUB_LOG_ERROR("GetBinHandleByFuncHandle failed for funcHandle=%p\n",
                      static_cast<void *>(funcHandle));
    return 0;
  }
  const auto &kernelInfo = MapManager::Instance().GetKernelInfo(binHandle);
  return GetFixedPcStartAddr(kernelInfo, targetKernelName, pcStartAddr);
}

// 将核函数所在内存设置为可读写，以支持断点设置
void EnableMemReadWrite(uint64_t pcStartAddr) {
  void *ptr = reinterpret_cast<void *>(pcStartAddr);
  uint64_t *base_ptr{};
  uint64_t psize{};
  auto ret = aclrtMemGetAddressRangeImpl(ptr, (void **)&base_ptr, &psize);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_WARNING(
        "aclrtMemGetAddressRange get addr size failed, "
        "If the memory used by your process is in a read-only state, "
        "it may lead to failure in setting breakpoints.\n");
    PrintErrorCode(ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR);
    return;
  }
  RT_STUB_LOG_INFO("pc_start_addr=%#lx,  base_addr=%#lx, psize=%lu\n",
                   pcStartAddr, (uint64_t)base_ptr, psize);
  int32_t deviceId{0};
  aclrtGetDeviceImpl(&deviceId);
  deviceId = ConvertToVisibleDeviceId(deviceId);
  SetMemAdviseIfNecessary((uint64_t)base_ptr, psize, deviceId);
}

uint64_t GetPcStartAddr(aclrtFuncHandle funcHandle) {
  uint64_t pcStartAddr = GetRawPcStartAddr(funcHandle);
  if (pcStartAddr == 0) {
    return 0;
  }

  pcStartAddr = FixPcStartAddr(funcHandle, pcStartAddr);
  if (pcStartAddr == 0) {
    return 0;
  }

  static std::string soc_version = aclrtGetSocNameImpl();
  // 950
  if (!StartsWith(soc_version, "Ascend950")) {
    return pcStartAddr;
  }

  EnableMemReadWrite(pcStartAddr);
  return pcStartAddr;
}

size_t ReadBinary(std::string const &filename, vector<uint8_t> &data)
{
    // check binPath validation
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        return 0;
    }
    ifs.seekg(0, std::ifstream::end);
    int64_t length = ifs.tellg();
    if (length <= 0) {
        return 0;
    }
    ifs.seekg(0, std::ifstream::beg);
    data.resize(length);
    ifs.read(reinterpret_cast<char *>(data.data()), length);
    return length;
}

void BinaryLoadFromFilePost(const char* binPath, aclrtBinHandle binHandle)
{
    if (binPath == nullptr) {
        return;
    }
    std::vector<uint8_t> data;
    ReadBinary(binPath, data);
    if (data.empty()) {
        return;
    }
    rtDevBinary_t bin{};
    bin.data = data.data();
    bin.length = data.size();
    string hash = GetSha256FromKernel(data.data(), data.size());
    string msg;
    if (!BinaryRegisterPost(&bin, binHandle, hash, msg)) {
        RT_STUB_LOG_WARNING("aclrtBinaryLoadFromFile postprocess failed: %s\n", msg.c_str());
    }
}

inline pid_t GetTgid()
{
    drvDeviceGetBareTgidFunc func =
        (drvDeviceGetBareTgidFunc)GetStubFuncPtr("drvDeviceGetBareTgid");

    pid_t tgid = func();
    RT_STUB_LOG_INFO("drvDeviceGetBareTgid done. tgid=%d, pid=%d\n", tgid, getpid());

    // 非容器场景，tgid与pid一致；容器场景，则不一致
    if (tgid == 0) {
        // 若 drvDeviceGetBareTgid 获取tgid异常，则使用普通pid
        tgid = getpid();
    }
    return tgid;
}

int32_t SetDevicePost(int32_t device)
{
    pid_t tgid = GetTgid();
    string socVersion{};
    const char *name = aclrtGetSocNameImpl();
    if (name != nullptr) {
        socVersion = name;
    }
    int32_t virtualDeviceId = 0;
    aclrtGetDeviceImpl(&virtualDeviceId);
    return SendDeviceInfo(device, virtualDeviceId, socVersion, tgid);
}

// 归一化 launch 接口第一个入参 func：可能是核函数符号，也可能是 func handle。
// 新版本 runtime 不区分二者，核函数名称与 func handle 均可直接用于下发，此时 func 可原样使用；
// 旧版本仅接受 func handle，传入核函数名称会报错。
// 用 aclrtFunctionGetBinaryImpl 探测当前版本新旧：成功 → 新版本（或 func 本身就是 handle），
// func 可原样使用；失败 → 旧版本且 func 为核函数名称，用 aclrtGetFuncBySymbolImpl 转成 handle。
aclrtFuncHandle ResolveFuncHandle(void *func) {
  if (func != nullptr) {
    using GetBinaryType = aclError (*)(aclrtFuncHandle, aclrtBinHandle *);
    auto getBinary = (GetBinaryType)GetStubFuncPtr("aclrtFunctionGetBinaryImpl", false);
    if (getBinary != nullptr) {
      aclrtBinHandle binHandle = nullptr;
      if (getBinary(static_cast<aclrtFuncHandle>(func), &binHandle) == ACL_SUCCESS) {
        RT_STUB_LOG_INFO(
            "Get binHandle by func success, func=%p is already aclrtFuncHandle "
            "type\n",
            func);
        return static_cast<aclrtFuncHandle>(func);
      }
    }
    using GetFuncBySymbolType = aclError (*)(const void *, aclrtFuncHandle *);
    auto impl = (GetFuncBySymbolType)GetStubFuncPtr("aclrtGetFuncBySymbolImpl", false);
    aclrtFuncHandle fh = nullptr;
    if (impl != nullptr && impl(func, &fh) == ACL_SUCCESS) {
      RT_STUB_LOG_INFO("Get func handle %p by symbol = %p success.\n", func,
                       static_cast<void *>(fh));
      return fh;
    }
  }
  RT_STUB_LOG_INFO("Receive param is nullptr or unresolvable, func=%p\n", func);
  return static_cast<aclrtFuncHandle>(func);
}

void LaunchKernelPre(aclrtFuncHandle funcHandle, aclrtStream stream)
{
  // 重置内存属性恢复信息，防止上一次launch的残留数据影响本次
  GetMemAdviseRestoreInfo() = {};
  // 打印launch info
  std::string kernelName = GetKernelNameFromStubFunc(funcHandle);
  kernelName = GetSimpleKernelName(kernelName);
  int32_t deviceId{0};
  aclrtGetDeviceImpl(&deviceId);
  ShowKernelLaunchInfo(kernelName, deviceId);
  // 获取pc_start_addr
  uint64_t pcStartAddr = GetPcStartAddr(funcHandle);
  if (pcStartAddr == 0) {
    RT_STUB_LOG_INFO("Get start pc for kernel=%.1024s failed, skip it",
                     kernelName.c_str());
    return;
  }

    ProcessAddrAsIpcMem(pcStartAddr);

    const auto binHandle = GetBinHandleByFuncHandle(funcHandle);
    if (binHandle == nullptr) {
      RT_STUB_LOG_ERROR("GetBinHandleByFuncHandle failed for funcHandle=%p\n",
                        static_cast<void *>(funcHandle));
      return;
    }
    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(binHandle);
    int32_t streamId{};
    auto ret = aclrtStreamGetIdImpl(stream, &streamId);
    if (ret != ACL_SUCCESS) {
      RT_STUB_LOG_WARNING("Get stream id failed, use streamId=0");
    }
    SendKernelInfo(kernelName, kernelInfo.kernelHash, kernelInfo.elf, pcStartAddr, streamId);
}

void LaunchKernelPost(aclrtStream stream)
{
    aclrtSynchronizeStreamImpl(stream);
    RestoreMemAdvise();
}

int32_t ConvertToVisibleDeviceIdIfPossible(int32_t devId)
{
    int32_t convertedId = -1;
    try {
        convertedId = ConvertToVisibleDeviceId(devId);
    } catch (...) {
      RT_STUB_LOG_INFO("Try to convert visible device id failed, try to use "
                       "aclrt api again.");
      auto ret = aclrtGetLogicDevIdByUserDevIdImpl(devId, &convertedId);
      if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_INFO("Try to convert visible device id failed, do not "
                         "set ASCEND_RT_VISIBLE_DEVICES env");
        convertedId = devId;
      }
    }
    return convertedId;
}

} // namespace

// 调用原始的halMemAdvise（非劫持版本），纯粹的接口调用
// 内部函数应调用此函数而非extern "C"的劫持版本halMemAdvise
drvError_t halMemAdviseOrigin(DVdeviceptr ptr, size_t count,
                              unsigned int advise, DVdevice device) {
  using FuncType = drvError_t (*)(DVdeviceptr, size_t, unsigned int, DVdevice);
  auto func = (FuncType)GetStubFuncPtr("halMemAdvise", false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find halMemAdvise failed\n");
    return 1;
  }
  return func(ptr, count, advise, device);
}

MemAdviseRestoreInfo &GetMemAdviseRestoreInfo() {
  static thread_local MemAdviseRestoreInfo inst{};
  return inst;
}

// 检查advise
// map，若已是预期值则跳过；否则保存原始值并调用halMemAdviseOrigin设置可读写
// 供aclrt_stub.cpp的EnableMemReadWrite和runtime_stub.cpp的SetMemoryWritable共享调用
void SetMemAdviseIfNecessary(uint64_t base_ptr, size_t psize,
                             int32_t deviceId) {
  constexpr unsigned int expectedAdvise = ADVISE_ACCESS_READWRITE;
  auto &adviseMap = GetHalMemAdviseMap();
  auto key = std::make_pair(base_ptr, (uint32_t)deviceId);
  auto it = adviseMap.find(key);
  if (it == adviseMap.end()) {
    // map中没有记录，说明该内存未被halMemAdvise过，默认为可读写，无需调用
    RT_STUB_LOG_INFO("halMemAdvise not set before, default read-write, "
                     "ptr=%#lx, device=%u, skip\n",
                     base_ptr, (uint32_t)deviceId);
    return;
  }
  if (it->second == expectedAdvise) {
    // advise已经是预期的值，无需再次调用halMemAdvise
    RT_STUB_LOG_INFO("halMemAdvise already set to %u, ptr=%#lx, device=%u, "
                     "skip\n",
                     expectedAdvise, base_ptr, (uint32_t)deviceId);
    return;
  }
  // 保存原来的advise值，用于LaunchKernelPost中恢复
  auto &restoreInfo = GetMemAdviseRestoreInfo();
  restoreInfo.ptr = base_ptr;
  restoreInfo.size = psize;
  restoreInfo.device = (uint32_t)deviceId;
  restoreInfo.originalAdvise = it->second;
  restoreInfo.needsRestore = true;
  drvError_t hal_ret =
      halMemAdviseOrigin(base_ptr, psize, expectedAdvise, deviceId);
  if (hal_ret != 0) {
    RT_STUB_LOG_WARNING(
        "halMemAdvise failed, ret=%u, device_id=%u, "
        "If the memory used by your process is in a read-only state, "
        "it may lead to failure in setting breakpoints.\n",
        hal_ret, deviceId);
    restoreInfo.needsRestore = false;
  }
}

// 恢复在kernel launch前修改的内存属性
// 如果不知道原来的属性值(needsRestore为false)，则不恢复
// 供aclrt_stub.cpp和runtime_stub.cpp的LaunchKernelPost共享调用
void RestoreMemAdvise() {
  auto &restoreInfo = GetMemAdviseRestoreInfo();
  if (!restoreInfo.needsRestore) {
    return;
  }
  RT_STUB_LOG_INFO("Restore halMemAdvise, ptr=%#lx, advise=%u, device=%u\n",
                   restoreInfo.ptr, restoreInfo.originalAdvise,
                   restoreInfo.device);
  halMemAdviseOrigin(restoreInfo.ptr, restoreInfo.size,
                     restoreInfo.originalAdvise, restoreInfo.device);
  restoreInfo = {};
}

/* ACLRUNTIME INSTRUMENTATION FUNCTION */

extern "C" {
// =================== aclruntime =====================
aclError aclrtBinaryLoadFromFileImpl(const char* binPath, aclrtBinaryLoadOptions *options, aclrtBinHandle *binHandle)
{
    using FuncType = decltype(&aclrtBinaryLoadFromFileImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(binPath, options, binHandle);
    if (ret == ACL_SUCCESS) {
        BinaryLoadFromFilePost(binPath, *binHandle);
    }
    return ret;
}

aclrtBinary aclrtCreateBinaryImpl(const void *data, size_t dataLen)
{
    using FuncType = decltype(&aclrtCreateBinaryImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    aclrtBinary ret = func(data, dataLen);
    auto &bin = GetDevBinaryMap()[ret];
    bin.data = data;
    bin.length = dataLen;
    return ret;
}

aclError aclrtBinaryLoadImpl(aclrtBinary binary, aclrtBinHandle *binHandle) {
  auto &bin = GetDevBinaryMap()[binary];
  string hash{};
  if (bin.data) {
    // calculate before register, may be changed after register
    hash =
        GetSha256FromKernel(static_cast<const uint8_t *>(bin.data), bin.length);
  }
  using FuncType = decltype(&aclrtBinaryLoadImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  auto ret = func(binary, binHandle);
  if (ret == ACL_SUCCESS && *binHandle) {
    string msg;
    if (!BinaryRegisterPost(&bin, *binHandle, hash, msg)) {
      RT_STUB_LOG_WARNING("%s postprocess failed: %s\n", __FUNCTION__,
                          msg.c_str());
    }
  }
  return ret;
}

aclError aclrtBinaryLoadFromDataImpl(const void *data, size_t length,
    const aclrtBinaryLoadOptions *options, aclrtBinHandle *binHandle)
{
    rtDevBinary_t bin{};
    bin.data = data;
    bin.length = length;
    string hash = GetSha256FromKernel(static_cast<const uint8_t *>(data), length);
    using FuncType = decltype(&aclrtBinaryLoadFromDataImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(data, length, options, binHandle);
    if (ret == ACL_SUCCESS && *binHandle) {
        string msg;
        if (!BinaryRegisterPost(&bin, *binHandle, hash, msg)) {
            RT_STUB_LOG_WARNING("%s postprocess failed: %s\n", __FUNCTION__, msg.c_str());
        }
    }
    return ret;
}

aclError aclrtBinaryGetFunctionImpl(aclrtBinHandle binHandle,
                                    const char *kernelName,
                                    aclrtFuncHandle *funcHandle) {
  using FuncType = decltype(&aclrtBinaryGetFunctionImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  auto ret = func(binHandle, kernelName, funcHandle);
  if (ret == ACL_SUCCESS && *funcHandle) {
    GetStubFuncPtrNameMap()[*funcHandle] = kernelName;
  }
  return ret;
}

aclError aclrtBinaryGetFunctionByEntryImpl(aclrtBinHandle binHandle, uint64_t funcEntry, aclrtFuncHandle *funcHandle)
{
    using FuncType = decltype(&aclrtBinaryGetFunctionByEntryImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(binHandle, funcEntry, funcHandle);
    if (ret == ACL_SUCCESS && *funcHandle) {
        std::string kernelName = GetKernelNameByTilingKey(static_cast<const void *>(binHandle), funcEntry);
        GetStubFuncPtrNameMap()[*funcHandle] = kernelName;
    }
    return ret;
}

aclError aclrtLaunchKernelWithConfigImpl(aclrtFuncHandle funcHandle,
    uint32_t blockDim, aclrtStream stream,
    aclrtLaunchKernelCfg *cfg, aclrtArgsHandle argsHandle, void *reserve)
{
    using FuncType = decltype(&aclrtLaunchKernelWithConfigImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    LaunchKernelPre(funcHandle, stream);
    auto ret = func(funcHandle, blockDim, stream, cfg, argsHandle, reserve);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchKernelWithHostArgsImpl(aclrtFuncHandle funcHandle,
        uint32_t blockDim, aclrtStream stream, aclrtLaunchKernelCfg *cfg,
        void *hostArgs, size_t argsSize, aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)
{
    using FuncType = decltype(&aclrtLaunchKernelWithHostArgsImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    LaunchKernelPre(funcHandle, stream);
    auto ret = func(funcHandle, blockDim, stream, cfg, hostArgs, argsSize, placeHolderArray, placeHolderNum);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchKernelV2Impl(aclrtFuncHandle funcHandle, uint32_t blockDim,
        const void *argsData, size_t argsSize, aclrtLaunchKernelCfg *cfg, aclrtStream stream)
{
    using FuncType = decltype(&aclrtLaunchKernelV2Impl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    LaunchKernelPre(funcHandle, stream);
    auto ret = func(funcHandle, blockDim, argsData, argsSize, cfg, stream);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchKernelImpl(aclrtFuncHandle funcHandle,
    uint32_t blockDim, const void *argsData, size_t argsSize, aclrtStream stream)
{
    using FuncType = decltype(&aclrtLaunchKernelImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    LaunchKernelPre(funcHandle, stream);
    auto ret = func(funcHandle, blockDim, argsData, argsSize, stream);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchSIMTKernelWithArgsArrayImpl(void *func, dim3 gridDim,
    dim3 blockDim, size_t dynUbufSize, aclrtStream stream,
    aclrtLaunchKernelCfg *cfg, void **args)
{
    using FuncType = decltype(&aclrtLaunchSIMTKernelWithArgsArrayImpl);
    auto origin = (FuncType)GetStubFuncPtr(__FUNCTION__);
    aclrtFuncHandle funcHandle = ResolveFuncHandle(func);
    LaunchKernelPre(funcHandle, stream);
    auto ret = origin(func, gridDim, blockDim, dynUbufSize, stream, cfg, args);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchKernelWithArgsArrayImpl(void *func, uint32_t numBlocks,
    aclrtStream stream, aclrtLaunchKernelCfg *cfg, void **args)
{
    using FuncType = decltype(&aclrtLaunchKernelWithArgsArrayImpl);
    auto origin = (FuncType)GetStubFuncPtr(__FUNCTION__);
    aclrtFuncHandle funcHandle = ResolveFuncHandle(func);
    LaunchKernelPre(funcHandle, stream);
    auto ret = origin(func, numBlocks, stream, cfg, args);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtLaunchSIMTKernelWithHostArgsImpl(void *func, dim3 gridDim,
    dim3 blockDim, size_t dynUbufSize, aclrtStream stream,
    aclrtLaunchKernelCfg *cfg, void *hostArgs, size_t argsSize,
    aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum)
{
    using FuncType = decltype(&aclrtLaunchSIMTKernelWithHostArgsImpl);
    auto origin = (FuncType)GetStubFuncPtr(__FUNCTION__);
    aclrtFuncHandle funcHandle = ResolveFuncHandle(func);
    LaunchKernelPre(funcHandle, stream);
    auto ret = origin(func, gridDim, blockDim, dynUbufSize, stream, cfg,
                      hostArgs, argsSize, placeHolderArray, placeHolderNum);
    if (ret == ACL_SUCCESS) {
        LaunchKernelPost(stream);
    }
    return ret;
}

aclError aclrtSetDeviceImpl(int32_t deviceId)
{
    LayerGuard guard(HijackedLayerManager::Instance(), __FUNCTION__);
    using FuncType = decltype(&aclrtSetDeviceImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("aclrtSetDeviceImpl device id=%d\n", deviceId);
    auto ret = func(deviceId);
    if (ret == ACL_SUCCESS) {
        SetDevicePost(ConvertToVisibleDeviceIdIfPossible(deviceId));
    }
    // unsupport visible current
    return ret;
}

aclError aclrtSynchronizeStreamWithTimeoutImpl(aclrtStream stream, int32_t timeout)
{
    LayerGuard guard(HijackedLayerManager::Instance(), __FUNCTION__);
    using FuncType = decltype(&aclrtSynchronizeStreamWithTimeoutImpl);
    RT_STUB_LOG_INFO("%s timeout=%d, pid=%u\n", __FUNCTION__, timeout, getpid());
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    // 为避免停在核函数中时间过长，出现流同步超时告警，因此检测到配置超时时间时替换为永不超时
    constexpr int32_t TIMEOUT_FOR_DEBUG = -1;
    auto ret = func(stream, TIMEOUT_FOR_DEBUG);
    if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    }
    return ret;
}

aclError aclrtSynchronizeDeviceWithTimeoutImpl(int32_t timeout) {
  LayerGuard guard(HijackedLayerManager::Instance(), __FUNCTION__);
  using FuncType = decltype(&aclrtSynchronizeDeviceWithTimeoutImpl);
  RT_STUB_LOG_INFO("%s timeout=%d, pid=%u\n", __FUNCTION__, timeout, getpid());
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  // 为避免停在核函数中时间过长，出现流同步超时告警，因此检测到配置超时时间时替换为永不超时
  constexpr int32_t TIMEOUT_FOR_DEBUG = -1;
  auto ret = func(TIMEOUT_FOR_DEBUG);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
  }
  return ret;
}

aclError aclrtCreateContextImpl(aclrtContext *context, int32_t deviceId)
{
    using FuncType = decltype(&aclrtCreateContextImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("%s device id=%d\n", __FUNCTION__, deviceId);
    rtError_t ret = func(context, deviceId);
    if (ret == ACL_SUCCESS) {
        SetDevicePost(ConvertToVisibleDeviceIdIfPossible(deviceId));
    }
    return ret;
}

aclError aclrtMemGetAddressRangeImpl(void *ptr, void **pbase, size_t *psize) {
  using FuncType = decltype(&aclrtMemGetAddressRangeImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(ptr, pbase, psize);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    ThrowErrorCode(ACLRT_GET_DEVICE_IMPL_FAILED_ERR);
  }
  return ret;
}

aclError aclrtIpcMemGetExportKeyImpl(void *devPtr, size_t size, char *key,
                                     size_t len, uint64_t flags) {
  using FuncType = decltype(&aclrtIpcMemGetExportKeyImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(devPtr, size, key, len, flags);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    ThrowErrorCode(ACLRT_IPC_MEM_GET_EXPORT_KEY_FAILED_ERR);
  }
  return ret;
}

aclError aclrtIpcMemCloseImpl(const char *key) {
  using FuncType = decltype(&aclrtIpcMemCloseImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(key);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    ThrowErrorCode(ACLRT_IPC_MEM_GET_EXPORT_KEY_FAILED_ERR);
  }
  return ret;
}

aclError aclrtMallocImpl(void **devPtr, size_t size,
                         aclrtMemMallocPolicy policy) {
  using FuncType = decltype(&aclrtMallocImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(devPtr, size, policy);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }

  ProcessAddrAsIpcMem((uint64_t)*devPtr);
  return ret;
}

aclError aclrtMallocAlign32Impl(void **devPtr, size_t size,
                                aclrtMemMallocPolicy policy) {
  using FuncType = decltype(&aclrtMallocAlign32Impl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(devPtr, size, policy);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }

  ProcessAddrAsIpcMem((uint64_t)*devPtr);
  return ret;
}

aclError aclrtMallocCachedImpl(void **devPtr, size_t size,
                               aclrtMemMallocPolicy policy) {
  using FuncType = decltype(&aclrtMallocCachedImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(devPtr, size, policy);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }

  ProcessAddrAsIpcMem((uint64_t)*devPtr);
  return ret;
}

aclError aclrtMallocWithCfgImpl(void **devPtr, size_t size,
                                aclrtMemMallocPolicy policy, void *cfg) {
  using FuncType = decltype(&aclrtMallocWithCfgImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }
  auto ret = func(devPtr, size, policy, cfg);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }

  ProcessAddrAsIpcMem((uint64_t)*devPtr);
  return ret;
}

aclError aclrtFreeImpl(void *devPtr) {
  using FuncType = decltype(&aclrtFreeImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }

  SendIpcMemFreeInfo((uint64_t)devPtr);

  auto ret = func(devPtr);
  return ret;
}

aclError aclrtFreeWithDevSyncImpl(void *devPtr) {
  using FuncType = decltype(&aclrtFreeWithDevSyncImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }

  SendIpcMemFreeInfo((uint64_t)devPtr);

  auto ret = func(devPtr);
  return ret;
}

drvError_t halMemAdvise(DVdeviceptr ptr, size_t count, unsigned int advise,
                        DVdevice device) {
  RT_STUB_LOG_INFO(
      "Enter halMemAdvise, ptr=%#lx, count=%lu, advise=%u, device=%u\n",
      (uint64_t)ptr, count, advise, (uint32_t)device);
  using FuncType = decltype(&halMemAdvise);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
  if (func == nullptr) {
    RT_STUB_LOG_ERROR("Find %s failed\n", __FUNCTION__);
    return 1;
  }

  auto ret = func(ptr, count, advise, device);
  if (ret == 0) {
    // 劫持函数保存/更新advise值到全局map，供其他函数判断使用
    GetHalMemAdviseMap()[{(uint64_t)ptr, (uint32_t)device}] = advise;
  }
  return ret;
}
}

#endif
