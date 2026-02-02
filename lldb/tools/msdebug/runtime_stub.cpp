/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER

#include "runtime_stub.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>
#include "acl.h"
#include "AscendCommunicationClient.h"
#include "HijackedLayerManager.h"
#include "lldb/Utility/AscendVerification.h"

using namespace std;

typedef rtError_t (*rtSetDeviceFunc)(int32_t);
typedef rtError_t (*rtGetDeviceFunc)(int32_t *);
typedef rtError_t (*rtSetDeviceExFunc)(int32_t);
typedef rtError_t (*rtCtxCreateExFunc)(void **ctx, uint32_t flags, int32_t device);
typedef rtError_t (*rtFunctionRegisterFunc)(void *, const void *, const char *,
    const void *, uint32_t);
typedef rtError_t (*rtGetFunctionByNameFunc)(const char *, void **);
typedef rtError_t (*rtGetAddrByFunFunc)(const void *, void **);
typedef rtError_t (*rtKernelLaunchFunc)(const void *, uint32_t, void *, uint32_t, rtSmDesc_t *, rtStream_t);
typedef rtError_t (*rtKernelLaunchWithFlagV2Func)(const void *, uint32_t,
    rtArgsEx_t *, rtSmDesc_t *, rtStream_t, uint32_t, const rtTaskCfgInfo_t *);
typedef rtError_t (*rtKernelLaunchWithHandleV2Func)(void *, const uint64_t,
    uint32_t, rtArgsEx_t *, rtSmDesc_t *, rtStream_t, const rtTaskCfgInfo_t *);
typedef rtError_t (*rtKernelGetAddrAndPrefCntFunc)(void *, const uint64_t,
    const void *const, const uint32_t, void **, uint32_t *);
typedef rtError_t (*rtGetSocVersion)(char *, const uint32_t);
typedef rtError_t (*rtDevBinaryRegisterFunc)(const rtDevBinary_t *, void **);
typedef rtError_t (*rtRegisterAllKernelFunc)(const rtDevBinary_t *, void **);
typedef rtError_t (*rtGetTaskIdAndStreamIDFunc)(uint32_t *taskId, uint32_t *streamId);
typedef rtError_t (*rtStreamSynchronizeWithTimeoutFunc)(rtStream_t stream, int32_t timeout);
typedef rtError_t (*rtStreamSynchronizeFunc)(rtStream_t stream);
typedef rtError_t (*rtGetVisibleDeviceIdByLogicDeviceIdFunc)(const int32_t, int32_t * const);

typedef pid_t (*drvDeviceGetBareTgidFunc)(void);

constexpr uint8_t SOC_VERSION_LEN = 32;
constexpr uint32_t BUFFER_SIZE = 2048; // lldb-server侧buffer大小
constexpr uint32_t KERNEL_NAME_SIZE = 2047; // lldb-client侧为2048
static void *GetStubFuncPtr(const std::string funcName);
static bool g_isInited = false;
static std::mutex g_initMtx;
static std::mutex g_initStubFunc;
static void *g_handle = nullptr;
static std::map<const void *, std::string> g_stubFuncPtrNameMap;

std::map<std::string, StubFuncInfo>& GetStubFuncInfoMap()
{
    static std::map<std::string, StubFuncInfo> stubFuncInfoMap;
    std::lock_guard<std::mutex> lk(g_initStubFunc);
    if (stubFuncInfoMap.size() == 0) {
        stubFuncInfoMap = {
            {"rtSetDevice",
                {"rtSetDevice", RT_SET_DEVICE_SYM_NOT_FOUND_ERR, nullptr}},
            {"rtSetDeviceEx",
                {"rtSetDeviceEx", RT_SET_DEVICE_EX_SYM_NOT_FOUND_ERR, nullptr}},
            {"rtCtxCreateEx",
                {"rtCtxCreateEx", RT_CTX_CREATE_EX_SYM_NOT_FOUND_ERR, nullptr}},
            {"rtFunctionRegister",
                {"rtFunctionRegister", RT_FUNC_REG_NOT_FOUND_ERR, nullptr}},
            {"rtKernelLaunch",
                {"rtKernelLaunch", RT_KERNEL_LAUNCH_NOT_FOUND_ERR, nullptr}},
            {"rtKernelLaunchWithFlagV2",
                {"rtKernelLaunchWithFlagV2", RT_KERNEL_LAUNCH_WITH_FLAG_V2_NOT_FOUND_ERR, nullptr}},
            {"rtKernelLaunchWithHandleV2",
                {"rtKernelLaunchWithHandleV2", RT_KERNEL_LAUNCH_WITH_HANDLE_V2_NOT_FOUND_ERR, nullptr}},
            {"rtKernelGetAddrAndPrefCnt",
                {"rtKernelGetAddrAndPrefCnt", RT_KERNEL_GET_ADDR_AND_PREF_CNT_NOT_FOUND_ERR, nullptr}},
            {"rtGetSocVersion",
                {"rtGetSocVersion", RT_GET_SOC_VERSION_NOT_FOUND_ERR, nullptr}},
            {"rtDevBinaryRegister",
                {"rtDevBinaryRegister", RT_DEV_BIN_REG_NOT_FOUND_ERR, nullptr}},
            {"rtRegisterAllKernel",
                {"rtRegisterAllKernel", RT_REG_ALL_KERNEL_NOT_FOUND_ERR, nullptr}},
            {"rtGetTaskIdAndStreamID",
                {"rtGetTaskIdAndStreamID", RT_GET_TASK_ID_AND_STREAM_ID_NOT_FOUND_ERR, nullptr}},
            {"rtStreamSynchronizeWithTimeout",
                {"rtStreamSynchronizeWithTimeout", RT_STREAM_SYNC_WITH_TIMEOUT_NOT_FOUND_ERR, nullptr}},
            {"rtStreamSynchronize",
                {"rtStreamSynchronize", RT_STREAM_SYNC_NOT_FOUND_ERR, nullptr}},
            {"rtGetVisibleDeviceIdByLogicDeviceId",
                {"rtGetVisibleDeviceIdByLogicDeviceId", RT_GET_VISIBLE_DEVID_BY_LOGIC_DEVID_NOT_FOUND_ERR, nullptr}},
            {"drvDeviceGetBareTgid",
                {"drvDeviceGetBareTgid", DRV_DEV_GET_BARE_TGID_NOT_FOUND_ERR, nullptr}},
            {"rtGetDevice",
                  {"rtGetDevice", RT_GET_DEVICE_NOT_FOUND_ERR, nullptr}},
        };
    }
    return stubFuncInfoMap;
}

static void GetDeviceId(int32_t* device)
{
    rtGetDeviceFunc func =
            (rtGetDeviceFunc)GetStubFuncPtr("rtGetDevice");
    rtError_t ret = func(device);
    if (ret != 0) {
        RT_STUB_LOG_ERROR("rtGetDevice failed. ret=%d\n", ret);
        ThrowErrorCode(RT_GET_DEVICE_FAILED_ERR);
    }
}

void ShowKernelLaunchInfo(const std::string &kernelName, int32_t deviceId)
{
    constexpr size_t MAX_SIZE = 64;
    auto simpleName = kernelName.substr(0, std::min(MAX_SIZE, kernelName.length()));
    printf("[Launch of Kernel %s on Device %d]\n", simpleName.c_str(), deviceId);
    fflush(stdout);
}

int32_t SendInfoAndWaitForReply(const std::string &buf)
{
    int32_t deviceId;
    GetDeviceId(&deviceId);
    RT_STUB_LOG_INFO("Native device id: %d, pid: %d, send message: %s\n", deviceId, getpid(), buf.c_str());
    string reply;
    auto &client = AscendCommunicationClient::GetInstance(deviceId);
    size_t ret = client.SendAndWaitResponse(buf, reply);
    if (ret <= 0) {
        return 0;
    }
    std::string msg = "";
    if (lldb_private::CheckStringValid(std::string(reply), msg)) {
        RT_STUB_LOG_ERROR("Invalid character: %s", msg.c_str());
        ThrowErrorCode(LLDB_REPLY_NOT_RECOGNIZED_ERR);
    }

    if (reply.substr(0, 4) == "$ok;") {
        return ret;
    }
    static const std::string startString("$fail:");
    size_t startPos = reply.find(startString);
    if (startPos != std::string::npos) {
        startPos = startString.size();
        size_t endPos = reply.find(";");
        std::string value = reply.substr(startPos, endPos - startPos);
        MSDEBUG_ERROR_CODE err_code;
        try {
            err_code = MSDEBUG_ERROR_CODE(stoi(value, 0, 16));
        } catch (...) {
            RT_STUB_LOG_ERROR("Invalid msdebug error code. reply = %s\n", reply.c_str());
            ThrowErrorCode(LLDB_REPLY_NOT_RECOGNIZED_ERR);
        }
        for (size_t i = 0U; i < (sizeof(ErrorTables) / sizeof(LLDBErrorCodeInfo)); i++) {
            if (ErrorTables[i].err_code == err_code) {
                printf("[ERROR] %s\n", ErrorTables[i].message.c_str());
                ThrowErrorCode(err_code);
            }
        }
    }
    RT_STUB_LOG_INFO("Reply = %s\n", reply.c_str());
    ThrowErrorCode(LLDB_REPLY_NOT_RECOGNIZED_ERR);
    return ret;
}

inline pid_t GetTgid()
{
    drvDeviceGetBareTgidFunc func_tid =
          (drvDeviceGetBareTgidFunc)GetStubFuncPtr("drvDeviceGetBareTgid");

    pid_t tgid = func_tid();
    RT_STUB_LOG_INFO("drvDeviceGetBareTgid done. tgid=%d, pid=%d\n", tgid, getpid());

    // 非容器场景，tgid与pid一致；容器场景，则不一致
    if (tgid == 0) {
      // 若 drvDeviceGetBareTgid 获取tgid异常，则使用普通pid
      tgid = getpid();
    }
    return tgid;
}

inline string GetSocName()
{
    char socVersion[SOC_VERSION_LEN] = {0};
    rtGetSocVersion func = (rtGetSocVersion)GetStubFuncPtr("rtGetSocVersion");
    rtError_t ret = func(socVersion, SOC_VERSION_LEN);
    if (ret != 0) {
        RT_STUB_LOG_ERROR("rtGetSocVersion failed. ret=%d\n", ret);
    }
    RT_STUB_LOG_INFO("SocVersion: %s\n", socVersion);
    return socVersion;
}

int32_t SendDeviceInfo(int32_t device, const std::string &socVersion, pid_t tgid)
{
    std::string buf = "$device_id:";
    buf += std::to_string(device);
    buf += ";";

    buf += "tgid:";
    buf += std::to_string(tgid);
    buf += ";";

    if (!socVersion.empty()) {
        buf += "soc_version:";
        buf += socVersion;
        buf += ";";
    }
    buf += "#";
    return SendInfoAndWaitForReply(buf);
}

static int32_t SetDevicePost(int32_t device)
{
    pid_t tgid = GetTgid();
    string socVersion = GetSocName();
    return SendDeviceInfo(device, socVersion, tgid);
}

static rtError_t GetStreamID(uint32_t &streamId)
{
  rtGetTaskIdAndStreamIDFunc func = (rtGetTaskIdAndStreamIDFunc)GetStubFuncPtr("rtGetTaskIdAndStreamID");
  uint32_t taskId;
  int32_t ret = func(&taskId, &streamId);
  if (ret != 0) {
      RT_STUB_LOG_ERROR("Get stream id failed!\n");
      PrintErrorCode(RT_KERNEL_GET_ADDR_AND_PREF_CNT_FAILED_ERR);
      return ret;
  }
  RT_STUB_LOG_INFO("rtGetTaskIdAndStreamID done. streamId=%u, taskId=%u\n", streamId, taskId);
  return ret;
}

static rtError_t rtStreamSynchronize(rtStream_t stream)
{
  auto func = (rtStreamSynchronizeFunc)GetStubFuncPtr("rtStreamSynchronize");
  auto ret = func(stream);
  if (ret != 0) {
      RT_STUB_LOG_ERROR("rtStreamSynchronize failed\n");
      return ret;
  }
  return ret;
}

int32_t SendStreamId(uint32_t streamId)
{
    std::string buf = "$stream_id:";
    buf += std::to_string(streamId);
    buf += ";#";
    RT_STUB_LOG_INFO("send buf=%s\n", buf.c_str());
    return SendInfoAndWaitForReply(buf);
}

static void LaunchKernelPost(rtStream_t stream)
{
    uint32_t streamId{0};
    if (GetStreamID(streamId) == 0) {
      SendStreamId(streamId);
    }
    rtStreamSynchronize(stream);
}

/*
 * Following the LLDB communication protocol,
 * escape the characters #, $, }, and *,
 * then use # as the message terminator so the peer can identify message boundaries.
 */
static string GetEscapedBytes(const vector<char> &raw) {
  string escapedBytes;
  escapedBytes.reserve(raw.size());
  const char *src = raw.data();
  size_t src_len = raw.size();
  while (src_len) {
    uint8_t byte = *src;
    src++;
    src_len--;
    // #, $, }, *
    if (byte == 0x23 || byte == 0x24 || byte == 0x7d || byte == 0x2a) {
      escapedBytes.push_back(0x7d);
      byte ^= 0x20;
    }
    escapedBytes.push_back(byte);
  };
  return escapedBytes;
}

int32_t SendKernelInfo(const std::string &kernelName, const std::string &kernelHash,
                       const std::vector<char> &elf, uint64_t pcAddr)
{
  static unordered_set<std::string> sentBinaries;
  string cutKernelName = kernelName;
  if (cutKernelName.length() > KERNEL_NAME_SIZE) {
    cutKernelName.resize(KERNEL_NAME_SIZE);
  }
  std::string buf = "$kernel_name:" + cutKernelName + ";";
  bool needSendBinary = false;
  if (sentBinaries.find(kernelHash) == sentBinaries.end()) {
    buf += "kernel_hash:" + kernelHash + ";";
    std::stringstream ss;
    ss << std::hex << pcAddr;
    buf += "pc_base_addr:" + ss.str() + ";";
    buf += "kernel_binary:" + GetEscapedBytes(elf) + ";";
    sentBinaries.insert(kernelHash);
    needSendBinary = true;
  }
  buf += "#";
  auto ret = SendInfoAndWaitForReply(buf);
  if (needSendBinary) {
    MSBreakOnLaunch();
  }
  return ret;
}

static void OpenRtLib()
{
    if (g_handle == nullptr) {
        char *toolkitPath = getenv("ASCEND_TOOLKIT_HOME");
        if (toolkitPath == nullptr) {
            ThrowErrorCode(ASCEND_TOOLKIT_HOME_NOT_FOUND_ERR);
        }
        std::string runtimeLibPath(toolkitPath);
        runtimeLibPath += "/lib64/libruntime.so";
        g_handle = dlopen(runtimeLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (g_handle == nullptr) {
            ThrowErrorCode(LIB_RUNTIME_NOT_FOUND_ERR);
        }
        RT_STUB_LOG_INFO("dlopen done\n");
    }

    for (auto &it : GetStubFuncInfoMap()) {
        it.second.funcPtr = dlsym(g_handle, it.second.name.c_str());
        if (it.second.funcPtr == nullptr) {
            ThrowErrorCode(it.second.errorCode);
        }
    }
}

void EnvCheck()
{
    if (access("/dev/drv_debug", F_OK) != 0) {
        printf("msdebug failed to initialize. "
        "please install HDK.\n");
        ThrowErrorCode(DRIVER_NOT_FOUND_ERR);
    }
    std::string debug_switch = "/proc/debug_switch";
    struct stat fileStat;
    if ((stat(debug_switch.c_str(), &fileStat) == 0) && (fileStat.st_mode & S_IRUSR) != 0) {
      std::ifstream file(debug_switch);
      if (file.is_open()) {
        std::string line;
        getline(file, line);
        file.close();
        if (line != "debug_switch_status = 1") {
          printf("msdebug failed to initialize, "
                  "please run \"echo 1 > /proc/debug_switch\" to turn on the switch.\n");
          ThrowErrorCode(DRIVER_NOT_FOUND_ERR);
        }
      }
    } else {
      RT_STUB_LOG_WARNING("/proc/debug_switch does not exist.\n");
    }
}

static void StubInit()
{
    std::unique_lock<std::mutex> lk(g_initMtx);
    if (g_isInited) {
        return;
    }

    try {
        RtStubLogger::Instance().Init();
    } catch (...) {
        RT_STUB_LOG_ERROR("Invalid value of $DEBUGGER_RT_STUB_LOG\n");
        ThrowErrorCode(ENV_VALUE_TYPE_ERR);
    }

    RT_STUB_LOG_INFO("LogInit done\n");

    OpenRtLib();

    EnvCheck();

    g_isInited = true;
    RT_STUB_LOG_INFO("StubInit done\n");
}

static string GetFullKernelName(const char *name, const vector<string> &fullNames)
{
    if (!name) {
        RT_STUB_LOG_WARNING("SubName is null.\n"); // 提示不合法
        return "";
    }
    if (!lldb_private::ContainsNullTerminator(name, BUFFER_SIZE)) {
        RT_STUB_LOG_WARNING("SubName is invalid (without terminator).\n"); // 提示不合法
        return "";
    }
    std::string msg = "";
    if (lldb_private::CheckStringValid(std::string(name), msg)) {
        RT_STUB_LOG_WARNING("SubName has invalid character %s.\n", msg.c_str()); // 提示不合法
        return "";
    }
    string subName(name);
    for (const auto &fullName: fullNames) {
        if (fullName == subName) {
            return fullName;
        }
        if (fullName.find(subName + MIX_AIC_TAIL) != fullName.npos) {
            return fullName;
        }
        if (fullName.find(subName + MIX_AIV_TAIL) != fullName.npos) {
            return fullName;
        }
    }
    return "";
}

static string GetFullStubName(void *binHandle, const char *stubName, const void *devFunc)
{
    auto kernelInfo = MapManager::Instance().GetKernelInfo(binHandle);
    string tmpName = GetFullKernelName(static_cast<const char *>(devFunc), kernelInfo.kernelNames);
    if (!tmpName.empty()) {
        return tmpName;
    }
    tmpName = GetFullKernelName(stubName, kernelInfo.kernelNames);
    if (!tmpName.empty()) {
        return tmpName;
    }
    return "anonymous";
}

static std::string GetKernelNameFromStubFunc(const void *stubFunc)
{
    auto it = g_stubFuncPtrNameMap.find(stubFunc);
    if (it == g_stubFuncPtrNameMap.end()) {
        RT_STUB_LOG_INFO("StubFunc is not found in map\n");
    } else {
        return it->second;
    }
    return "anonymous";
}

std::string GetKernelNameByTilingKey(const void *hdl, uint64_t tilingKey)
{
    std::string nameSuffix = "_" + std::to_string(tilingKey);
    std::vector<std::string> suffixNames = {nameSuffix, nameSuffix + MIX_AIC_TAIL, nameSuffix + MIX_AIV_TAIL};
 
    auto kernelInfo = MapManager::Instance().GetKernelInfo(hdl);
    for (const auto &name: kernelInfo.kernelNames) {
        for (const auto &suffix: suffixNames) {
            if (EndsWith(name, suffix)) {
                return name;
            }
        }
    }
    return "";
}

static void *GetStubFuncPtr(const std::string funcName)
{
    RT_STUB_LOG_INFO("GetStubFuncPtr funcName=%s\n", funcName.c_str());
    auto it = GetStubFuncInfoMap().find(funcName);
    if (it == GetStubFuncInfoMap().end()) {
        ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
    }
    if (it->second.funcPtr == nullptr) {
        StubInit();
    }
    if (it->second.funcPtr == nullptr) {
        ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
    }
    // if StubInit() returns, all stub functions are supposed to be loaded
    return it->second.funcPtr;
}

uint64_t GetFixedPcStartAddr(const KernelInfo &kernelInfo,
    const std::string &targetKernelName, uint64_t pcStartAddr)
{
    uint64_t kernelOffset = UINT64_MAX;
    for (size_t i = 0; i < kernelInfo.kernelNames.size(); i++) {
        const auto &kernelName = kernelInfo.kernelNames[i];
        if (kernelName == targetKernelName) {
            kernelOffset = kernelInfo.kernelOffsets[i];
            break;
        }
    }
    if (kernelOffset == UINT64_MAX) {
        RT_STUB_LOG_ERROR("Can not find kernel name in kernel info.");
        kernelOffset = 0;
    }
    if (pcStartAddr <= kernelOffset) {
        RT_STUB_LOG_ERROR("pcStartAddr=%#lx <= kernelOffset=%#lx", pcStartAddr, kernelOffset);
        return 0;
    }
    pcStartAddr -= kernelOffset;
    RT_STUB_LOG_INFO("kernel_name=%s, kernel_offset=0x%lx, return pc_start_addr=0x%lx\n",
                targetKernelName.c_str(), kernelOffset, pcStartAddr);
    return pcStartAddr;
}

static uint64_t GetPcStartAddrDynamic(void *hdl, const uint64_t tilingKey)
{
    rtKernelGetAddrAndPrefCntFunc func =
        (rtKernelGetAddrAndPrefCntFunc)GetStubFuncPtr("rtKernelGetAddrAndPrefCnt");

    // hdl and tilingKey are needed in dynamic scenario
    uint64_t pcStartAddr;
    uint32_t prefetchCnt;
    int32_t ret = func(hdl, tilingKey, nullptr, 1U,
        (void **)&pcStartAddr, &prefetchCnt);
    if (ret != 0) {
        PrintErrorCode(RT_KERNEL_GET_ADDR_AND_PREF_CNT_FAILED_ERR);
        return 0;
    }

    std::string targetKernelName = GetKernelNameByTilingKey(hdl, tilingKey);
    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(hdl);
    return GetFixedPcStartAddr(kernelInfo, targetKernelName, pcStartAddr);
}

static uint64_t GetPcStartAddrStatic(const void *stubFunc)
{
    rtKernelGetAddrAndPrefCntFunc func =
        (rtKernelGetAddrAndPrefCntFunc)GetStubFuncPtr("rtKernelGetAddrAndPrefCnt");

    uint64_t pcStartAddr;
    uint32_t prefetchCnt;

    int32_t ret = func(nullptr, 0U, stubFunc, 0U, (void **)&pcStartAddr, &prefetchCnt);
    RT_STUB_LOG_INFO("rtKernelGetAddrAndPrefCnt done. pc_start_addr=0x%lx stubFunc_addr=0x%lx\n",
        pcStartAddr, (uint64_t)stubFunc);
    if (ret != 0) {
        // AICPU may failed
        PrintErrorCode(RT_KERNEL_GET_ADDR_AND_PREF_CNT_FAILED_ERR);
        return 0;
    }

    std::string targetKernelName = GetKernelNameFromStubFunc(stubFunc);
    const void *handle = MapManager::Instance().GetHandle(stubFunc);
    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(handle);
    return GetFixedPcStartAddr(kernelInfo, targetKernelName, pcStartAddr);
}

bool BinaryRegisterPost(const rtDevBinary_t *bin, void *hdl, const string &hash, string &err)
{
    if (!bin || !bin->data || !hdl) {
        err = "empty binary data or handle.";
        return false;
    }
    RT_STUB_LOG_INFO("magic=%u, version=%u, length=%lu pid=%u\n",
                bin->magic, bin->version, bin->length, getpid());

    KernelInfo kernelInfo{};
    if (!GenKernelInfo(bin->data, bin->length, kernelInfo)) {
        err = "get kernel symbol info failed.";
        return false;
    }
    kernelInfo.kernelHash = hash;
    RT_STUB_LOG_INFO("Get kernel hash = %s, get %lu kernelNames\n",
                kernelInfo.kernelHash.c_str(), kernelInfo.kernelNames.size());
    // check size
    const char *binData = static_cast<const char *>(bin->data);
    kernelInfo.elf.assign(binData, binData + bin->length);

    MapManager::Instance().AddKernelInfoMap(hdl, kernelInfo);
    return true;
}

int32_t ConvertToVisibleDeviceId(int32_t devId)
{
    // device id captured in hijacked reSetDevice is not always correct if
    // ASCEND_RT_VISIBLE_DEVICES is set. conversion needs to be done here.

    rtGetVisibleDeviceIdByLogicDeviceIdFunc func = (rtGetVisibleDeviceIdByLogicDeviceIdFunc)
        GetStubFuncPtr("rtGetVisibleDeviceIdByLogicDeviceId");
    int32_t convertedId = -1;
    int32_t ret = func(devId, &convertedId);
    RT_STUB_LOG_INFO("rtGetVisibleDeviceIdByLogicDeviceId done. ret=%d convertedId=%d\n",
        ret, convertedId);
    if (ret != 0) {
        return devId;
    } else {
        return convertedId;
    }
}

/* RUNTIME INSTRUMENTATION FUNCTION */

extern "C" {
rtError_t rtSetDevice(int32_t device)
{
    rtSetDeviceFunc func = (rtSetDeviceFunc)GetStubFuncPtr(__FUNCTION__);
    if (HijackedLayerManager::Instance().InCallStack(__FUNCTION__)) {
        return func(device);
    }

    RT_STUB_LOG_INFO("rtSetDevice device id=%d\n", device);
    rtError_t ret = func(device);
    if (ret == 0) {
        SetDevicePost(ConvertToVisibleDeviceId(device));
    }
    return ret;
}

rtError_t rtSetDeviceEx(int32_t device)
{
    rtSetDeviceExFunc func = (rtSetDeviceExFunc)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("rtSetDeviceEx device id=%d\n", device);
    rtError_t ret = func(device);
    if (ret == 0) {
        SetDevicePost(ConvertToVisibleDeviceId(device));
    }
    return ret;
}

rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device)
{
    rtCtxCreateExFunc func = (rtCtxCreateExFunc)GetStubFuncPtr(__FUNCTION__);
    if (HijackedLayerManager::Instance().InCallStack(__FUNCTION__)) {
        return func(ctx, flags, device);
    }
    RT_STUB_LOG_INFO("rtCtxCreateEx device id=%d\n", device);
    rtError_t ret = func(ctx, flags, device);
    if (ret == 0) {
        SetDevicePost(ConvertToVisibleDeviceId(device));
    }
    return ret;
}

rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc,
    const char *stubName, const void *devFunc, uint32_t funcMode)
{
    // stubName validation verification
    rtFunctionRegisterFunc func = (rtFunctionRegisterFunc)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("rtFunctionRegister stubFunc_addr=0x%lx\n", (uint64_t)stubFunc);
    g_stubFuncPtrNameMap[stubFunc] = GetFullStubName(binHandle, stubName, devFunc);
    MapManager::Instance().AddFuncHandleMap(stubFunc, binHandle);
    return func(binHandle, stubFunc, stubName, devFunc, funcMode);
}

rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim,
    void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream)
{
    // for <<<>>> scenario
    rtKernelLaunchFunc func = (rtKernelLaunchFunc)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("rtKernelLaunch pid=%u\n", getpid());

    // 打印launch info
    std::string kernelName = GetKernelNameFromStubFunc(stubFunc);
    kernelName = GetSimpleKernelName(kernelName);
    int device{};
    GetDeviceId(&device);
    ShowKernelLaunchInfo(kernelName, device);

    const void *handle = MapManager::Instance().GetHandle(stubFunc);
    uint64_t pcStartAddr = GetPcStartAddrStatic(stubFunc);

    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(handle);
    SendKernelInfo(kernelName, kernelInfo.kernelHash, kernelInfo.elf, pcStartAddr);

    rtError_t result = func(stubFunc, blockDim, args, argsSize, smDesc, stream);
    if (result == 0) {
        LaunchKernelPost(stream);
    }
    return result;
}

rtError_t rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                                 rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo)
{
    // for static register-function scenario
    rtKernelLaunchWithFlagV2Func func = (rtKernelLaunchWithFlagV2Func)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("rtKernelLaunchWithFlagV2 pid=%u\n", getpid());

    // 打印launch info
    std::string kernelName = GetKernelNameFromStubFunc(stubFunc);
    int device{};
    GetDeviceId(&device);
    ShowKernelLaunchInfo(kernelName, device);

    const void *handle = MapManager::Instance().GetHandle(stubFunc);
    uint64_t pcStartAddr = GetPcStartAddrStatic(stubFunc);

    const auto &kernelInfo = MapManager::Instance().GetKernelInfo(handle);
    SendKernelInfo(kernelName, kernelInfo.kernelHash, kernelInfo.elf, pcStartAddr);

    rtError_t result = func(stubFunc, blockDim, argsInfo, smDesc, stm, flags, cfgInfo);
    if (result == 0) {
        LaunchKernelPost(stm);
    }
    return result;
}

rtError_t rtKernelLaunchWithHandle(void *hdl, const uint64_t tilingKey, uint32_t blockDim, rtArgsEx_t *argsInfo,
    rtSmDesc_t *smDesc, rtStream_t stm, const void* kernelInfo)
{
    return rtKernelLaunchWithHandleV2(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, nullptr);
}

rtError_t rtKernelLaunchWithHandleV2(void *hdl, const uint64_t tilingKey,
    uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm,
    const rtTaskCfgInfo_t *cfgInfo)
{
    rtKernelLaunchWithHandleV2Func func = (rtKernelLaunchWithHandleV2Func)GetStubFuncPtr(__FUNCTION__);
    RT_STUB_LOG_INFO("rtKernelLaunchWithHandleV2 pid=%u\n", getpid());

    std::string kernelName = GetKernelNameByTilingKey(hdl, tilingKey);
    kernelName = GetSimpleKernelName(kernelName);
    int device{};
    GetDeviceId(&device);
    ShowKernelLaunchInfo(kernelName, device);

    // 获取pc_start_addr
    uint64_t pcStartAddr = GetPcStartAddrDynamic(hdl, tilingKey);

    if (pcStartAddr) {
      const auto &kernelInfo = MapManager::Instance().GetKernelInfo(hdl);
      SendKernelInfo(kernelName, kernelInfo.kernelHash, kernelInfo.elf, pcStartAddr);
    }

    rtError_t result = func(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, cfgInfo);
    if (result == 0) {
        LaunchKernelPost(stm);
    }
    return result;
}

rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **hdl)
{
    // NOTE: must calculate before register
    string hash{};
    if (bin && bin->data) {
        hash = GetSha256FromKernel(static_cast<const uint8_t *>(bin->data), bin->length);
    }
    rtDevBinaryRegisterFunc func = (rtDevBinaryRegisterFunc)GetStubFuncPtr(__FUNCTION__);
    rtError_t ret = func(bin, hdl);
    if (ret != 0) {
        RT_STUB_LOG_ERROR("rtDevBinaryRegister failed. ret=%d\n", ret);
        return ret;
    }
    if (!hdl) {
        return ret;
    }
    RT_STUB_LOG_INFO("enter rtDevBinaryRegister Post.\n");
    string msg;
    if (!BinaryRegisterPost(bin, *hdl, hash, msg)) {
        RT_STUB_LOG_ERROR("rtDevBinaryRegister postprocess failed: %s\n", msg.c_str());
    }
    return ret;
}

rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void
**handle)
{
    // NOTE: must calculate before register
    string hash{};
    if (bin && bin->data) {
        hash = GetSha256FromKernel(static_cast<const uint8_t *>(bin->data), bin->length);
    }

    rtRegisterAllKernelFunc func = (rtRegisterAllKernelFunc)GetStubFuncPtr(__FUNCTION__);
    rtError_t ret = func(bin, handle);
    if (ret != 0) {
        RT_STUB_LOG_ERROR("rtRegisterAllKernel failed. ret=%d\n", ret);
        return ret;
    }
    if (!handle) {
        return ret;
    }

    RT_STUB_LOG_INFO("Enter rtRegisterAllKernel Post.\n");
    string msg;
    if (!BinaryRegisterPost(bin, *handle, hash, msg)) {
        RT_STUB_LOG_ERROR("rtRegisterAllKernel postprocess failed: %s\n", msg.c_str());
    }
    return ret;
}

rtError_t rtStreamSynchronizeWithTimeout(rtStream_t stream, int32_t timeout)
{
    rtStreamSynchronizeWithTimeoutFunc func =
        (rtStreamSynchronizeWithTimeoutFunc)GetStubFuncPtr(__FUNCTION__);
    if (HijackedLayerManager::Instance().InCallStack(__FUNCTION__)) {
        return func(stream, timeout);
    }

    RT_STUB_LOG_INFO("rtStreamSynchronizeWithTimeout timeout=%d, pid=%u\n",
        timeout, getpid());

    // 为避免停在核函数中时间过长，出现流同步超时告警，因此检测到配置超时时间时替换为永不超时
    constexpr int32_t TIMEOUT_FOR_DEBUG = -1;

    rtError_t ret = func(stream, TIMEOUT_FOR_DEBUG);
    if (ret != 0) {
        RT_STUB_LOG_ERROR("rtStreamSynchronizeWithTimeout failed. ret=%d\n", ret);
    }
    return ret;
}

/*
 * This function is an empty stub used by the tool for internal breakpoints. 
 * When a breakpoint is hit in this function, the debugger will update the kernel binary,
 * match the preset breakpoints, and apply them. Additionally, this function must not be inlined.
 */
void MSBreakOnLaunch()
{
    RT_STUB_LOG_INFO("Enter MSBreakOnLaunch\n");
}
}
#endif
