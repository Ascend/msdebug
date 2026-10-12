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
