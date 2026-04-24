/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef ASCEND_DEBUG_ACL_H
#define ASCEND_DEBUG_ACL_H

#include <cstddef>
#include <cstdint>
#include <string>

#define VISIBILITY_EXPORT __attribute__((visibility("default")))

typedef enum aclrtLaunchKernelAttrId {
    ACL_RT_LAUNCH_KERNEL_ATTR_SCHEM_MODE = 1,  // 调度模式
} aclrtLaunchKernelAttrId;
 
typedef union aclrtLaunchKernelAttrValue {
    uint8_t schemMode;
    uint32_t localMemorySize;
    uint32_t rsv[4];
} aclrtLaunchKernelAttrValue;
 
typedef struct aclrtLaunchKernelAttr {
    aclrtLaunchKernelAttrId id;
    aclrtLaunchKernelAttrValue value;
} aclrtLaunchKernelAttr;
 
typedef struct aclrtLaunchKernelCfg {
    aclrtLaunchKernelAttr *attrs;
    size_t numAttrs;
} aclrtLaunchKernelCfg;

typedef struct {
    uint32_t addrOffset;
    uint32_t dataOffset;
} aclrtPlaceHolderInfo;

typedef int aclError;
typedef void* aclrtStream;
typedef void* aclrtEvent;
 
typedef void* aclrtBinHandle;
typedef void* aclrtFuncHandle;
typedef void* aclrtParamHandle;
typedef void* aclrtBinary;
typedef void* aclrtArgsHandle;
typedef void* aclrtContext;
typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

typedef UINT32 DVmem_advise;
typedef UINT32 DVdevice;
typedef UINT64 DVdeviceptr;
typedef uint32_t drvError_t;
typedef drvError_t DVresult;

enum ADVISE_MEM_TYPE {
    ADVISE_PERSISTENT = 0,
    ADVISE_DEV_MEM = 1,
    ADVISE_ACCESS_READONLY = 2,
    ADVISE_ACCESS_READWRITE = 3,
};

static const int ACL_SUCCESS = 0;
 
typedef enum aclrtBinaryLoadOptionType {
    ACL_RT_BINARY_LOAD_OPT_LAZY_LOAD = 1, // 指定解析算子二进制、注册算子后，是否加载算子到Device侧
    ACL_RT_LOAD_BINARY_OPT_MAGIC = 2,     // 标识算子类型的魔术数字
} aclrtBinaryLoadOptionType;
 
typedef union aclrtBinaryLoadOptionValue {
    uint32_t isLazyLoad;
    uint32_t magic;
    uint32_t rsv[4];
} aclrtBinaryLoadOptionValue;
 
typedef struct {
    aclrtBinaryLoadOptionType type;
    aclrtBinaryLoadOptionValue value;
} aclrtBinaryLoadOption;
 
typedef struct aclrtBinaryLoadOptions {
    aclrtBinaryLoadOption *options;
    size_t numOpt;
} aclrtBinaryLoadOptions;

inline bool StartsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

extern "C" {
VISIBILITY_EXPORT aclError aclrtBinaryLoadFromFileImpl(
    const char* binPath, aclrtBinaryLoadOptions *options, aclrtBinHandle *binHandle);
 
VISIBILITY_EXPORT aclrtBinary aclrtCreateBinaryImpl(const void *data, size_t dataLen);
VISIBILITY_EXPORT aclError aclrtBinaryLoadImpl(const aclrtBinary binary, aclrtBinHandle *binHandle);
 
VISIBILITY_EXPORT aclError aclrtBinaryGetFunctionImpl(const aclrtBinHandle binHandle,
    const char *kernelName, aclrtFuncHandle *funcHandle);
VISIBILITY_EXPORT aclError aclrtBinaryGetFunctionByEntryImpl(
    aclrtBinHandle binHandle, uint64_t funcEntry, aclrtFuncHandle *funcHandle);
 
VISIBILITY_EXPORT aclError aclrtLaunchKernelWithConfigImpl(aclrtFuncHandle funcHandle, uint32_t blockDim,
    aclrtStream stream, aclrtLaunchKernelCfg *cfg, aclrtArgsHandle argsHandle, void *reserve);
VISIBILITY_EXPORT aclError aclrtLaunchKernelImpl(aclrtFuncHandle funcHandle,
    uint32_t blockDim, const void *argsData, size_t argsSize, aclrtStream stream);
VISIBILITY_EXPORT aclError aclrtCreateContextImpl(aclrtContext *context, int32_t deviceId);
VISIBILITY_EXPORT aclError aclrtSetDeviceImpl(int32_t deviceId);

VISIBILITY_EXPORT aclError aclrtGetFunctionAddrImpl(aclrtFuncHandle funcHandle, void **aicAddr, void **aivAddr);
VISIBILITY_EXPORT aclError aclrtSynchronizeStreamWithTimeoutImpl(aclrtStream stream, int32_t timeout);

VISIBILITY_EXPORT aclError aclrtLaunchKernelWithHostArgsImpl(aclrtFuncHandle funcHandle,
        uint32_t blockDim, aclrtStream stream, aclrtLaunchKernelCfg *cfg,
        void *hostArgs, size_t argsSize, aclrtPlaceHolderInfo *placeHolderArray, size_t placeHolderNum);
 
VISIBILITY_EXPORT aclError aclrtLaunchKernelV2Impl(aclrtFuncHandle funcHandle, uint32_t blockDim,
        const void *argsData, size_t argsSize, aclrtLaunchKernelCfg *cfg, aclrtStream stream);
 
VISIBILITY_EXPORT aclError aclrtBinaryLoadFromDataImpl(const void *data, size_t length,
    const aclrtBinaryLoadOptions *options, aclrtBinHandle *binHandle);

}

#endif
#endif
