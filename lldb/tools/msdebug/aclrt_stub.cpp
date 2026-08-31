/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER

#include "HijackedLayerManager.h"
#include "acl.h"
#include "elf_symbol_check.h"
#include "runtime_stub.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
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

void *GetStubFuncPtr(const std::string funcName, bool throw_error=true);
void *g_handle = nullptr;
void *g_drvHandle = nullptr;

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

std::map<std::string, StubFuncInfo>& GetAclrtStubFuncInfoMap()
{
    static std::map<std::string, StubFuncInfo> stubFuncInfoMap;
    if (stubFuncInfoMap.size() == 0) {
      stubFuncInfoMap = {
          {"aclrtGetDeviceImpl",
           {"aclrtGetDeviceImpl", ACLRT_GET_DEVICE_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtSetDeviceImpl",
           {"aclrtSetDeviceImpl", ACLRT_SET_DEVICE_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtCreateContextImpl",
           {"aclrtCreateContextImpl", ACLRT_CREATE_CONTEXT_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtBinaryGetFunctionImpl",
           {"aclrtBinaryGetFunctionImpl",
            ACLRT_BINARY_GET_FUNCTION_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtBinaryGetFunctionByEntryImpl",
           {"aclrtBinaryGetFunctionByEntryImpl",
            ACLRT_BINARY_GET_FUNCTION_BY_ENTRY_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtLaunchKernelImpl",
           {"aclrtLaunchKernelImpl", ACLRT_LAUNCH_KERNEL_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtLaunchKernelWithConfigImpl",
           {"aclrtLaunchKernelWithConfigImpl",
            ACLRT_LAUNCH_KERNEL_WITH_CONFIG_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtLaunchKernelWithHostArgsImpl",
           {"aclrtLaunchKernelWithHostArgsImpl",
            ACLRT_LAUNCH_KERNEL_WITH_HOST_ARGS_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtLaunchKernelV2Impl",
           {"aclrtLaunchKernelV2Impl",
            ACLRT_LAUNCH_KERNEL_V2_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtLaunchSIMTKernelWithArgsArrayImpl",
           {"aclrtLaunchSIMTKernelWithArgsArrayImpl",
            ACLRT_LAUNCH_SIMT_KERNEL_WITH_ARGS_ARRAY_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtLaunchKernelWithArgsArrayImpl",
           {"aclrtLaunchKernelWithArgsArrayImpl",
            ACLRT_LAUNCH_KERNEL_WITH_ARGS_ARRAY_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtLaunchSIMTKernelWithHostArgsImpl",
           {"aclrtLaunchSIMTKernelWithHostArgsImpl",
            ACLRT_LAUNCH_SIMT_KERNEL_WITH_HOST_ARGS_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtGetFunctionAddrImpl",
           {"aclrtGetFunctionAddrImpl",
            ACLRT_GET_FUNCTION_ADDR_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtGetSocNameImpl",
           {"aclrtGetSocNameImpl", ACLRT_GET_SOC_NAME_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtBinaryLoadFromFileImpl",
           {"aclrtBinaryLoadFromFileImpl",
            ACLRT_BINARY_LOAD_FROM_FILE_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtCreateBinaryImpl",
           {"aclrtCreateBinaryImpl", ACLRT_CREATE_BINARY_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtBinaryLoadImpl",
           {"aclrtBinaryLoadImpl", ACLRT_BINARY_LOAD_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtBinaryLoadFromDataImpl",
           {"aclrtBinaryLoadFromDataImpl",
            ACLRT_BINARY_LOAD_FROM_DATA_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtStreamGetIdImpl",
           {"aclrtStreamGetIdImpl", ACLRT_STREAM_GET_ID_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtSynchronizeStreamImpl",
           {"aclrtSynchronizeStreamImpl", ACLRT_SYNC_STREAM_IMPL_NOT_FOUND_ERR,
            nullptr}},
          {"aclrtMemGetAddressRangeImpl",
           {"aclrtMemGetAddressRangeImpl",
            ACLRT_MEM_GET_ADDRESS_RANGE_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtGetLogicDevIdByUserDevIdImpl",
           {"aclrtGetLogicDevIdByUserDevIdImpl",
            ACLRT_GET_LOGIC_DEVID_BY_USER_DEVID_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtFunctionGetBinaryImpl",
           {"aclrtFunctionGetBinaryImpl",
            ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtGetFunctionNameImpl",
           {"aclrtGetFunctionNameImpl",
             ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtGetFuncBySymbolImpl",
           {"aclrtGetFuncBySymbolImpl",
             ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtSynchronizeDeviceWithTimeoutImpl",
           {"aclrtSynchronizeDeviceWithTimeoutImpl",
            ACLRT_SYNCHRONIZE_DEVICE_WITH_TIMEOUT_NOT_FOUND_ERR, nullptr}},
          {"aclrtSynchronizeStreamWithTimeoutImpl",
           {"aclrtSynchronizeStreamWithTimeoutImpl",
            ACLRT_SYNC_STREAM_WITH_TIMEOUT_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtIpcMemGetExportKeyImpl",
           {"aclrtIpcMemGetExportKeyImpl",
            ACLRT_IPC_MEM_GET_EXPORT_KEY_FAILED_ERR, nullptr}},
          {"aclrtIpcMemCloseImpl",
           {"aclrtIpcMemCloseImpl", ACLRT_IPC_MEM_CLOSE_FAILED_ERR, nullptr}},
          {"aclrtMallocImpl",
           {"aclrtMallocImpl", ACLRT_MALLOC_FAILED_ERR, nullptr}},
          {"aclrtMallocAlign32Impl",
           {"aclrtMallocAlign32Impl", ACLRT_MALLOC_ALIGN_32_FAILED_ERR,
            nullptr}},
          {"aclrtMallocCachedImpl",
           {"aclrtMallocCachedImpl", ACLRT_MALLOC_CACHED_FAILED_ERR, nullptr}},
          {"aclrtMallocWithCfgImpl",
           {"aclrtMallocWithCfgImpl", ACLRT_MALLOC_WITH_CFG_FAILED_ERR,
            nullptr}},
          {"aclrtFreeImpl", {"aclrtFreeImpl", ACLRT_FREE_FAILED_ERR, nullptr}},
          {"aclrtFreeWithDevSyncImpl",
           {"aclrtFreeWithDevSyncImpl", ACLRT_FREE_WITH_DEV_SYNC_FAILED_ERR,
            nullptr}},
      };
    }
    return stubFuncInfoMap;
}

std::map<std::string, StubFuncInfo>& GetDrvStubFuncInfoMap()
{
    static std::map<std::string, StubFuncInfo> stubFuncInfoMap;
    if (stubFuncInfoMap.size() == 0) {
        stubFuncInfoMap = {
            {"drvDeviceGetBareTgid",
                {"drvDeviceGetBareTgid", DRV_DEV_GET_BARE_TGID_NOT_FOUND_ERR, nullptr}},
            {"halMemAdvise",
                {"halMemAdvise", HAL_MEM_ADVISE_NOT_FOUND_ERR, nullptr}},
        };
    }
    return stubFuncInfoMap;
}

std::map<std::string, StubFuncInfo>& GetStubFuncInfoMap()
{
    static std::map<std::string, StubFuncInfo> stubFuncInfoMap;
    static std::mutex stubMapInitMtx;
    std::lock_guard<std::mutex> lk(stubMapInitMtx);
    if (stubFuncInfoMap.size() == 0) {
        auto stubMap = GetAclrtStubFuncInfoMap();
        stubFuncInfoMap.insert(stubMap.begin(), stubMap.end());
        stubMap = GetDrvStubFuncInfoMap();
        stubFuncInfoMap.insert(stubMap.begin(), stubMap.end());
    }
    return stubFuncInfoMap;
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

void OpenAclrtLib() {
  if (g_handle == nullptr) {
    char *toolkitPath = getenv("ASCEND_TOOLKIT_HOME");
    if (toolkitPath == nullptr) {
      ThrowErrorCode(ASCEND_TOOLKIT_HOME_NOT_FOUND_ERR);
    }
    std::string soName = GetAclRuntimeLibName(toolkitPath);
    RT_STUB_LOG_INFO("will open so name %s\n", soName.c_str());
    std::string runtimeLibPath(toolkitPath);
    runtimeLibPath += "/lib64/lib" + soName + ".so";
    g_handle = dlopen(runtimeLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (g_handle == nullptr) {
      std::string oldAclImplPath(toolkitPath);
      oldAclImplPath += "/lib64/libascendcl_impl.so";
      RT_STUB_LOG_INFO("dlopen %s failed, change to open %s\n",
                       runtimeLibPath.c_str(), oldAclImplPath.c_str());
      g_handle = dlopen(oldAclImplPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
      if (g_handle == nullptr) {
        ThrowErrorCode(LIB_ACL_RUNTIME_IMPL_NOT_FOUND_ERR);
      }
    }
    RT_STUB_LOG_INFO("dlopen aclrt impl done\n");
  }

  for (auto &it : GetAclrtStubFuncInfoMap()) {
    it.second.funcPtr = dlsym(g_handle, it.second.name.c_str());
    if (it.second.funcPtr == nullptr) {
      RT_STUB_LOG_WARNING(
          "dlsym %s failed, may cause error if user's program required.",
          it.second.name.c_str());
      continue;
    }
    GetStubFuncInfoMap()[it.first] = it.second;
  }
  if (GetStubFuncInfoMap().empty()) {
    ThrowErrorCode(DLSYM_ALL_STUBFUNC_ERROR);
  }
}

void OpenDrvLib()
{
    if (g_drvHandle == nullptr) {
        g_drvHandle = dlopen("libascend_hal.so", RTLD_NOW | RTLD_GLOBAL);
        if (g_drvHandle == nullptr) {
            ThrowErrorCode(LIB_ASCEND_HAL_NOT_FOUND_ERR);
        }
        RT_STUB_LOG_INFO("dlopen hal done\n");
    }

    for (auto &it : GetDrvStubFuncInfoMap()) {
        it.second.funcPtr = dlsym(g_drvHandle, it.second.name.c_str());
        if (it.second.funcPtr == nullptr) {
            RT_STUB_LOG_WARNING("dlsym %s failed, may cause error if user's program required.",
                                it.second.name.c_str());
            continue;
        }
        GetStubFuncInfoMap()[it.first] = it.second;
    }
}

void StubInit()
{
    static std::mutex initMtx;
    static bool isInited = false;
    std::unique_lock<std::mutex> lk(initMtx);
    if (isInited) {
        return;
    }

    try {
        RtStubLogger::Instance().Init();
    } catch (...) {
        RT_STUB_LOG_ERROR("Invalid value of $DEBUGGER_RT_STUB_LOG");
        ThrowErrorCode(ENV_VALUE_TYPE_ERR);
    }

    RT_STUB_LOG_INFO("LogInit done\n");

    OpenAclrtLib();
    OpenDrvLib();

    EnvCheck();

    isInited = true;
    RT_STUB_LOG_INFO("AclrtStubInit done\n");
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
    RT_STUB_LOG_INFO("GetStubFuncPtr funcName=%s\n", funcName.c_str());
    auto it = GetStubFuncInfoMap().find(funcName);
    if (it == GetStubFuncInfoMap().end()) {
        if (throw_error) {
            ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
        }
        return nullptr;
    }
    if (it->second.funcPtr == nullptr) {
        StubInit();
    }
    if (it->second.funcPtr == nullptr) {
        if (throw_error) {
            ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
        }
        return nullptr;
    }
    // if StubInit() returns, all stub functions are supposed to be loaded
    return it->second.funcPtr;
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
// 先尝试 aclrtGetFuncBySymbolImpl(func, &fh)：成功 → func 是核函数符号，用解析出的 handle；
// 失败 → func 本身就是 handle，直接使用。
aclrtFuncHandle ResolveFuncHandle(void *func) {
  aclrtFuncHandle fh = nullptr;
  if (func != nullptr) {
    using FuncType = aclError (*)(const void *, aclrtFuncHandle *);
    auto impl = (FuncType)GetStubFuncPtr("aclrtGetFuncBySymbolImpl");
    if (impl(func, &fh) == ACL_SUCCESS) {
      RT_STUB_LOG_INFO("Receive param is kernel symbol, func=%p, "
                       "resolved funcHandle=%p\n",
                       func, static_cast<void *>(fh));
      return fh;
    }
  }
  RT_STUB_LOG_INFO("Receive param is funcHandle, func=%p\n", func);
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
