/*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*/

#ifdef MS_DEBUGGER

#ifndef RT_STUB_LOG_H
#define RT_STUB_LOG_H

#include <cstdlib>
#include <string>
#include "Singleton.h"

enum MSDEBUG_ERROR_CODE {
    /* 初始化错误 */
    // 环境变量配置错误
    ASCEND_TOOLKIT_HOME_NOT_FOUND_ERR = 0x10000, // $ASCEND_TOOLKIT_HOME 未找到
    SOCKET_PATH_NOT_FOUND_ERR = 0x10001, // $SOCKET_PATH 未找到
    ENV_VALUE_TYPE_ERR = 0x10002, // 环境变量类型非法

    // rt/drv接口符号加载错误
    LIB_RUNTIME_NOT_FOUND_ERR = 0x10100, // 打开runtime库文件失败
    RT_SET_DEVICE_SYM_NOT_FOUND_ERR = 0x10101, // rtSetDevice
    RT_SET_DEVICE_EX_SYM_NOT_FOUND_ERR = 0x10102, // rtSetDeviceEx
    RT_CTX_CREATE_EX_SYM_NOT_FOUND_ERR = 0x10103, // rtCtxCreateEx
    RT_GET_FUNC_BY_NAME_NOT_FOUND_ERR = 0x10104, // rtGetFunctionByName
    RT_GET_ADDR_BY_FUN_NOT_FOUND_ERR = 0x10105, // rtGetAddrByFun
    RT_FUNC_REG_NOT_FOUND_ERR = 0x10106, // rtFunctionRegister
    RT_KERNEL_LAUNCH_NOT_FOUND_ERR = 0x10107, // rtKernelLaunch
    RT_KERNEL_LAUNCH_WITH_FLAG_V2_NOT_FOUND_ERR = 0x10108, // rtKernelLaunchWithFlagV2
    RT_KERNEL_LAUNCH_WITH_HANDLE_V2_NOT_FOUND_ERR = 0x10109, // rtKernelLaunchWithHandleV2
    RT_KERNEL_GET_ADDR_AND_PREF_CNT_NOT_FOUND_ERR = 0x1010A, // rtKernelGetAddrAndPrefCnt
    RT_GET_SOC_VERSION_NOT_FOUND_ERR = 0x1010B, // rtGetSocVersion
    RT_DEV_BIN_REG_NOT_FOUND_ERR = 0x1010C, // rtDevBinaryRegister
    RT_REG_ALL_KERNEL_NOT_FOUND_ERR = 0x1010D, // rtRegisterAllKernel
    RT_GET_TASK_ID_AND_STREAM_ID_NOT_FOUND_ERR = 0x1010E, // rtGetTaskIdAndStreamID
    RT_STREAM_SYNC_WITH_TIMEOUT_NOT_FOUND_ERR = 0x1010F, // rtStreamSynchronizeWithTimeoutFunc
    RT_GET_VISIBLE_DEVID_BY_LOGIC_DEVID_NOT_FOUND_ERR = 0x10110, // rtGetVisibleDeviceIdByLogicDeviceId
    RT_GET_DEVICE_NOT_FOUND_ERR = 0x10111, // rtGetDevice

    LIB_ACL_RUNTIME_IMPL_NOT_FOUND_ERR = 0x1011E, // 打开ascendcl_impl库文件失败
    LIB_ASCEND_HAL_NOT_FOUND_ERR = 0x1011F, // 打开ascend_hal库文件失败
    DRV_DEV_GET_BARE_TGID_NOT_FOUND_ERR = 0x10120, // drvDeviceGetBareTgid
    ACLRT_GET_DEVICE_IMPL_NOT_FOUND_ERR = 0x10121, // aclrtGetDeviceImpl
    ACLRT_SET_DEVICE_IMPL_NOT_FOUND_ERR = 0x10122, // aclrtSetDeviceImpl
    ACLRT_CREATE_CONTEXT_IMPL_NOT_FOUND_ERR = 0x10123, // aclrtCreateContextImpl
    ACLRT_BINARY_GET_FUNCTION_IMPL_NOT_FOUND_ERR = 0x10124, // aclrtBinaryGetFunctionImpl
    ACLRT_BINARY_GET_FUNCTION_BY_ENTRY_IMPL_NOT_FOUND_ERR = 0x10125, // aclrtBinaryGetFunctionByEntryImpl
    ACLRT_LAUNCH_KERNEL_IMPL_NOT_FOUND_ERR = 0x10126, // aclrtLaunchKernelImpl
    ACLRT_LAUNCH_KERNEL_WITH_CONFIG_IMPL_NOT_FOUND_ERR = 0x10127, // aclrtLaunchKernelWithConfigImpl
    ACLRT_GET_FUNCTION_ADDR_IMPL_NOT_FOUND_ERR = 0x10128, // aclrtGetFunctionAddrImpl
    ACLRT_GET_SOC_NAME_IMPL_NOT_FOUND_ERR = 0x10129, // aclrtGetSocNameImpl
    ACLRT_BINARY_LOAD_FROM_FILE_IMPL_NOT_FOUND_ERR = 0x1012A, // aclrtBinaryLoadFromFileImpl
    ACLRT_CREATE_BINARY_IMPL_NOT_FOUND_ERR = 0x1012B, // aclrtCreateBinaryImpl
    ACLRT_BINARY_LOAD_IMPL_NOT_FOUND_ERR = 0x1012C, // aclrtBinaryLoadImpl
    ACLRT_STREAM_GET_ID_IMPL_NOT_FOUND_ERR = 0x1012D, // aclrtStreamGetIdImpl
    ACLRT_SYNC_STREAM_WITH_TIMEOUT_IMPL_NOT_FOUND_ERR = 0x1012E, // aclrtSynchronizeStreamWithTimeoutImpl
    ACLRT_LAUNCH_KERNEL_WITH_HOST_ARGS_IMPL_NOT_FOUND_ERR = 0x1012F, // aclrtLaunchKernelWithHostArgsImpl
    ACLRT_LAUNCH_KERNEL_V2_IMPL_NOT_FOUND_ERR = 0x10130, // aclrtLaunchKernelV2Impl
    ACLRT_BINARY_LOAD_FROM_DATA_IMPL_NOT_FOUND_ERR = 0x1013A, // aclrtBinaryLoadFromDataImpl
    // 其他错误
    ALLOC_ALREADY_OCCUPIED_DEVICE_ID_ERR = 0x10200, // 使用的device id已被占用
    RT_SET_DIFFERENT_DEVICE_ID_ERR = 0x10201, // 检测到多次set device，且前后id不一致
    ACCESS_INVALID_RT_FUNC_NAME_ERR = 0x10202, // 尝试获取无效的rt函数指针
    GET_KERNEL_INFO_ERROR = 0x10203,  // get kernel info failed, handle is not saved when register function is running
    STUBFUNC_VECTOR_EMPTY_ERROR = 0x10204, // stubFunc Vector is empty, can not get pc start

    /* 运行时错误 */
    // rt接口返回异常
    RT_GET_FUNC_BY_NAME_FAILED_ERR = 0x20000, // rtGetFunctionByName返回异常
    RT_GET_ADDR_BY_FUN_FAILED_ERR = 0x20001, // rtGetAddrByFun返回异常
    RT_KERNEL_GET_ADDR_AND_PREF_CNT_FAILED_ERR = 0x20002, // rtKernelGetAddrAndPrefCnt返回异常
    RT_GET_DEVICE_FAILED_ERR = 0x20003, // rtGetDeviceId返回异常
    RT_GET_SOC_VERSION_FAILED_ERR = 0x20004, // rtGetSocVersion返回异常
                                        //
    ACLRT_GET_FUNCTION_ADDR_IMPL_FAILED_ERR = 0x20005, // aclrtGetFunctionAddrImpl 返回异常
    ACLRT_STREAM_GET_ID_IMPL_FAILED_ERR = 0x20006, // aclrtStreamGetIdImpl返回异常
    ACLRT_GET_DEVICE_IMPL_FAILED_ERR = 0x20007, // aclrtGetDeviceImpl返回异常

    // 其他错误
    LLDB_REPLY_NOT_RECOGNIZED_ERR = 0x20100, // lldb的回复字符串异常
    RT_FUNC_NAME_NOT_FOUND = 0x20101, // 未找到kernel名
    DRIVER_NOT_FOUND_ERR = 0x20102, // 未找到调试KO
    LLDB_GET_KERNEL_OFFSET_ERR = 0x20103, // 解析kernel偏移量错误

    // Init LLDB TO TS ERROR
    OPEN_KO_ERR = 0x20200,
    SET_BREAKPOINT_ERR = 0x20201,
    INIT_DEBUG_MODE_ERR = 0x20202,
    INTERNAL_DEBUGGER_ERR = 0x20203,
    UNSUPPORTED_SOC_TYPE_ERR = 0x20204,
    INVALID_DEVICE_INFO_ERR = 0x20205,
    INVALID_KERNEL_INFO_ERR = 0x20206,
    INVALID_STREAM_ID_ERR = 0x20207,
    INVALID_HEADER_ERR = 0x20208,

    // send kernel name error
    SEND_KERNEL_NAME_ERROR = 0x20301,
};

struct LLDBErrorCodeInfo {
  MSDEBUG_ERROR_CODE err_code;
  std::string message;
};

const LLDBErrorCodeInfo ErrorTables[] = {
  {OPEN_KO_ERR, "Initialize msdebug failed, maybe device is already occupied by another msdebug program."},
  {SET_BREAKPOINT_ERR, "Set breakpoint failed."},
  {INIT_DEBUG_MODE_ERR, "Initialize debug mode failed."},
  {INTERNAL_DEBUGGER_ERR, "There is a msdebug internal problem."},
  {UNSUPPORTED_SOC_TYPE_ERR, "This soc type is unsupported."},
  {INVALID_DEVICE_INFO_ERR, "Invalid device info value."},
  {INVALID_KERNEL_INFO_ERR, "Invalid kernel info value."},
  {INVALID_STREAM_ID_ERR, "Invalid stream id value."},
  {INVALID_HEADER_ERR, "Invalid message header."}
};

inline void ThrowErrorCode(MSDEBUG_ERROR_CODE code)
{
  printf("[ERROR] error code: 0x%x\n", code);
  throw code;
}

#define RT_STUB_LOG(...)                                                             \
    do {                                                                             \
        if (RtStubLogger::Instance().IsLogEnable()) {                                \
            printf(__VA_ARGS__);                                                     \
            fflush(stdout);                                                          \
        }                                                                            \
    } while (0)

#define RT_STUB_LOG_ERROR(...)                                                       \
    do {                                                                             \
          RT_STUB_LOG("[ERROR]" __VA_ARGS__);                                        \
    } while (0)

#define RT_STUB_LOG_WARNING(...)                                                     \
    do {                                                                             \
          RT_STUB_LOG("[WARNING]" __VA_ARGS__);                                      \
    } while (0)

#define RT_STUB_LOG_INFO(...)                                                        \
    do {                                                                             \
          RT_STUB_LOG("[INFO]" __VA_ARGS__);                                         \
    } while (0)

class RtStubLogger : public Singleton<RtStubLogger, false> {
public:
  void Init() {
    char *buf = secure_getenv("DEBUGGER_RT_STUB_LOG");
    if (buf != nullptr) {
      std::string logStr(buf);
      logEnable_ = std::stoi(logStr);
    }
  }

  bool IsLogEnable() {
    return logEnable_;
  }

private:
  bool logEnable_ = false;
};

inline void PrintErrorCode(MSDEBUG_ERROR_CODE code)
{
  RT_STUB_LOG_WARNING("error code: 0x%x", code);
}


#endif // RT_STUB_LOG_H
#endif
