/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <map>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <unistd.h>

#include "acl.h"
#include "HijackedLayerManager.h"
#include "runtime_stub.h"

using namespace std;

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

static std::map<const void *, std::pair<const void *, std::string>> &GetSymbolToBinMap()
{
    static std::map<const void *, std::pair<const void *, std::string>> inst{};
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
          {"aclrtGetUserDevIdByLogicDevIdImpl",
           {"aclrtGetUserDevIdByLogicDevIdImpl",
            ACLRT_GET_USER_DEVID_BY_LOGIC_DEVID_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtGetFuncBySymbolImpl",
           {"aclrtGetFuncBySymbolImpl",
            ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR, nullptr}},
          {"aclrtSynchronizeStreamWithTimeoutImpl",
           {"aclrtSynchronizeStreamWithTimeoutImpl",
            ACLRT_SYNC_STREAM_WITH_TIMEOUT_IMPL_NOT_FOUND_ERR, nullptr}}};
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

aclError aclrtMemGetAddressRangeImpl(void *ptr, void **pbase, size_t *psize)
{
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

aclError aclrtGetUserDevIdByLogicDevIdImpl(const int32_t logicDevId,
                                           int32_t *const userDevid) {
  using FuncType = decltype(&aclrtGetUserDevIdByLogicDevIdImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  auto ret = func(logicDevId, userDevid);
  if (ret != ACL_SUCCESS) {
    RT_STUB_LOG_ERROR("%s failed. ret=%d\n", __FUNCTION__, ret);
    return ret;
  }
  return ret;
}

void OpenAclrtLib()
{
    if (g_handle == nullptr) {
        char *toolkitPath = getenv("ASCEND_TOOLKIT_HOME");
        if (toolkitPath == nullptr) {
            ThrowErrorCode(ASCEND_TOOLKIT_HOME_NOT_FOUND_ERR);
        }
        std::string runtimeLibPath(toolkitPath);
        runtimeLibPath += "/lib64/libacl_rt_impl.so";
        g_handle = dlopen(runtimeLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (g_handle == nullptr) {
            std::string oldAclImplPath(toolkitPath);
            oldAclImplPath += "/lib64/libascendcl_impl.so";
            RT_STUB_LOG_INFO("dlopen %s failed, change to open %s\n", runtimeLibPath.c_str(), oldAclImplPath.c_str());
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
            RT_STUB_LOG_WARNING("dlsym %s failed, may cause error if user's program required.",
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
    if (it == stubFuncNameMap.end()) {
        RT_STUB_LOG_INFO("stubFunc is not found in map\n");
    } else {
        return it->second;
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

drvError_t halMemAdvise(DVdeviceptr ptr, size_t count, unsigned int advise, DVdevice device)
{
    RT_STUB_LOG_INFO("Enter %s, ptr=%#llx, count=%lu, advise=%u\n", __FUNCTION__, ptr, count, advise);
    using FuncType = decltype(&halMemAdvise);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__, false);
    if (func == nullptr) {
        return 1;
    }
    return func(ptr, count, advise, device);
}

const char *aclrtGetSocNameImpl()
{
    using FuncType = decltype(&aclrtGetSocNameImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    return func();
}

uint64_t GetPcStartAddr(aclrtFuncHandle funcHandle)
{
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
    uint64_t pcStartAddr{};
    if (aicAddr) {
        pcStartAddr = reinterpret_cast<uint64_t>(aicAddr);
    } else if (aivAddr) {
        pcStartAddr = reinterpret_cast<uint64_t>(aivAddr);
    } else {
        RT_STUB_LOG_ERROR("aclrtGetFunctionAddrImpl get addr all zero\n");
        PrintErrorCode(ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR);
        return 0;
    }

    std::string targetKernelName = GetKernelNameFromStubFunc(funcHandle);
    const void *handle = MapManager::Instance().GetHandle(funcHandle);

    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(handle);
    pcStartAddr = GetFixedPcStartAddr(kernelInfo, targetKernelName, pcStartAddr);
    static std::string soc_version = aclrtGetSocNameImpl();
    // 950
    if (!StartsWith(soc_version, "Ascend950")) {
      return pcStartAddr;
    }
    void *ptr = reinterpret_cast<void *>(pcStartAddr);
    uint64_t *base_ptr{};
    uint64_t psize{};
    ret = aclrtMemGetAddressRangeImpl(ptr, (void**)&base_ptr, &psize);
    if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_WARNING("aclrtMemGetAddressRange get addr size failed, "
                "If the memory used by your process is in a read-only state, "
                "it may lead to failure in setting breakpoints.\n");
        PrintErrorCode(ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR);
        return pcStartAddr;
    }
    RT_STUB_LOG_INFO("pc_start_addr=%#lx,  base_addr=%#lx, psize = %lu", pcStartAddr, (uint64_t)base_ptr, psize);
    int32_t deviceId{0};
    aclrtGetDeviceImpl(&deviceId);
    if (halMemAdvise(pcStartAddr, psize, 3, deviceId) != 0) {
        RT_STUB_LOG_WARNING("halMemAdvise failed, "
                "If the memory used by your process is in a read-only state, "
                "it may lead to failure in setting breakpoints.\n");
        return pcStartAddr;
    }
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
    return SendDeviceInfo(device, socVersion, tgid);
}

void LaunchKernelPre(aclrtFuncHandle funcHandle, aclrtStream stream)
{
    // 打印launch info
    std::string kernelName = GetKernelNameFromStubFunc(funcHandle);
    kernelName = GetSimpleKernelName(kernelName);
    int32_t deviceId{0};
    aclrtGetDeviceImpl(&deviceId);
    ShowKernelLaunchInfo(kernelName, deviceId);
    // 获取pc_start_addr
    uint64_t pcStartAddr = GetPcStartAddr(funcHandle);
    if (pcStartAddr == 0) {
      RT_STUB_LOG_INFO("Get start pc for kernel=%.1024s failed, skip it", kernelName.c_str());
      return;
    }
    const auto *binHandle = MapManager::Instance().GetHandle(funcHandle);
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
}

int32_t ConvertToVisibleDeviceIdIfPossible(int32_t devId)
{
    int32_t convertedId = -1;
    try {
        convertedId = ConvertToVisibleDeviceId(devId);
    } catch (...) {
      RT_STUB_LOG_INFO("Try to convert visible device id failed, try to use "
                       "aclrt api again.");
      auto ret = aclrtGetUserDevIdByLogicDevIdImpl(devId, &convertedId);
      if (ret != ACL_SUCCESS) {
        RT_STUB_LOG_INFO("Try to convert visible device id failed, do not "
                         "set ASCEND_RT_VISIBLE_DEVICES env");
        convertedId = devId;
      }
    }
    return convertedId;
}

} // namespace

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

aclError aclrtBinaryLoadImpl(aclrtBinary const binary, aclrtBinHandle *binHandle)
{
    auto &bin = GetDevBinaryMap()[binary];
    string hash{};
    if (bin.data) {
        // calculate before register, may be changed after register
        hash = GetSha256FromKernel(static_cast<const uint8_t *>(bin.data), bin.length);
    }
    using FuncType = decltype(&aclrtBinaryLoadImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(binary, binHandle);
    if (ret == ACL_SUCCESS && *binHandle) {
        string msg;
        if (!BinaryRegisterPost(&bin, *binHandle, hash, msg)) {
            RT_STUB_LOG_WARNING("%s postprocess failed: %s\n", __FUNCTION__, msg.c_str());
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

aclError aclrtBinaryGetFunctionImpl(aclrtBinHandle const binHandle,
        const char *kernelName, aclrtFuncHandle *funcHandle)
{
    using FuncType = decltype(&aclrtBinaryGetFunctionImpl);
    auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
    auto ret = func(binHandle, kernelName, funcHandle);
    if (ret == ACL_SUCCESS && *funcHandle) {
        GetStubFuncPtrNameMap()[*funcHandle] = kernelName;
        MapManager::Instance().AddFuncHandleMap(*funcHandle, binHandle);
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
        MapManager::Instance().AddFuncHandleMap(*funcHandle, binHandle);
    }
    return ret;
}

aclError aclrtGetFuncBySymbolImpl(const void *symbol,
                                  aclrtFuncHandle *funcHandle) {
  using FuncType = decltype(&aclrtGetFuncBySymbolImpl);
  auto func = (FuncType)GetStubFuncPtr(__FUNCTION__);
  auto ret = func(symbol, funcHandle);
  if (ret == ACL_SUCCESS && *funcHandle) {
    auto info = GetSymbolToBinMap()[symbol];
    auto *binHandle = info.first;
    if (binHandle) {
        GetStubFuncPtrNameMap()[*funcHandle] = info.second;
        MapManager::Instance().AddFuncHandleMap(*funcHandle, binHandle);
    }
  }
  return ret;
}

// only rt
rtError_t rtRegisterFuncSymbol(void *binHandle, const void *symbol, const char *kernelName, void *reserve) {
  RT_STUB_LOG_INFO("Enter rtRegisterFuncSymbol\n");
  auto ret = rtRegisterFuncSymbolOrigin(binHandle, symbol, kernelName, reserve);
  if (ret == ACL_SUCCESS && binHandle && kernelName) {
    GetSymbolToBinMap()[symbol] = {binHandle, kernelName};
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

}

#endif
