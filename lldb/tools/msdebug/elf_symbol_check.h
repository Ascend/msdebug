/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */

#ifndef LLDB_TOOLS_MSDEBUG_ELF_SYMBOL_CHECK_H
#define LLDB_TOOLS_MSDEBUG_ELF_SYMBOL_CHECK_H

#include <cstdint>
#include <elf.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// 不通过 dlopen 加载 SO，而是直接解析 ELF 文件的 .dynsym 节来查符号，
// 避免触发 SO 构造函数导致的全局状态副作用。libruntime.so 是可信文件，
// 无需做边界/魔数校验。
inline bool HasSymbolInSo(const std::string &soPath,
                          const std::string &symbolName) {
  int fd = open(soPath.c_str(), O_RDONLY);
  if (fd < 0) {
    return false;
  }
  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    return false;
  }
  size_t fileSize = static_cast<size_t>(st.st_size);
  void *mapped = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (mapped == MAP_FAILED) {
    return false;
  }

  auto *base = static_cast<uint8_t *>(mapped);
  auto *ehdr = reinterpret_cast<Elf64_Ehdr *>(base);
  auto *shdr = reinterpret_cast<Elf64_Shdr *>(base + ehdr->e_shoff);
  for (size_t i = 0; i < ehdr->e_shnum; i++) {
    if (shdr[i].sh_type != SHT_DYNSYM) {
      continue;
    }
    const char *strings =
        reinterpret_cast<const char *>(base + shdr[shdr[i].sh_link].sh_offset);
    auto *symbols = reinterpret_cast<Elf64_Sym *>(base + shdr[i].sh_offset);
    size_t numSymbols = shdr[i].sh_size / sizeof(Elf64_Sym);
    for (size_t j = 0; j < numSymbols; j++) {
      if (symbolName == (strings + symbols[j].st_name)) {
        munmap(mapped, fileSize);
        return true;
      }
    }
  }
  munmap(mapped, fileSize);
  return false;
}

// 运行时动态检测 libruntime.so 是否导出 aclrtSetDeviceImpl，判断新版/旧版场景：
// - 新版：libruntime.so 自身导出 aclrtSetDeviceImpl
// - 旧版：走原有 "acl_rt_impl" / "ascendcl_impl" 逻辑
inline std::string GetAclRuntimeLibName(const std::string &toolkitPath) {
  std::string runtimeSoPath = toolkitPath + "/lib64/libruntime.so";
  if (HasSymbolInSo(runtimeSoPath, "aclrtSetDeviceImpl")) {
    return "runtime";
  }
  return "acl_rt_impl";
}

#endif
