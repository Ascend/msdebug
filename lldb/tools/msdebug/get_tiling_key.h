/*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef LLVM_GET_TILLING_KEY_H
#define LLVM_GET_TILLING_KEY_H

#include <cstdint>
#include <string>
#include <vector>

constexpr char const *MIX_AIC_TAIL = "_mix_aic";
constexpr char const *MIX_AIV_TAIL = "_mix_aiv";

struct KernelInfo {
    std::string kernelHash;
    std::vector<std::string> kernelNames;
    std::vector<uint64_t> kernelOffsets;
    std::vector<char> elf;
};
std::string GetSha256FromKernel(const uint8_t *data, uint64_t len);

bool GenKernelInfo(const void* data, size_t dataSize, KernelInfo &kernlInfo);

std::string GetSimpleKernelName(const std::string &fullName);

bool EndsWith(const std::string &fullStr, const std::string &ending);
#endif
#endif
