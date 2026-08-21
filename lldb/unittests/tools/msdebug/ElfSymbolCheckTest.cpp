/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */

#include "elf_symbol_check.h"

#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

// 构造一个仅含 .dynstr/.dynsym 节的最小 ELF64 文件，导出 symbols 中的符号。
std::vector<uint8_t>
BuildElfWithDynsyms(const std::vector<std::string> &symbols) {
  // 字符串表：首字节为 '\0'，其后依次放置各符号名。
  std::string strtab;
  strtab.push_back('\0');
  std::vector<uint32_t> nameOffsets;
  nameOffsets.reserve(symbols.size());
  for (const auto &name : symbols) {
    nameOffsets.push_back(static_cast<uint32_t>(strtab.size()));
    strtab += name;
    strtab.push_back('\0');
  }

  constexpr size_t kSectionCount = 3; // null + .dynstr + .dynsym
  const size_t ehdrSize = sizeof(Elf64_Ehdr);
  const size_t shdrSize = sizeof(Elf64_Shdr);
  const size_t symSize = sizeof(Elf64_Sym);
  const size_t symCount = symbols.size() + 1; // 首符号为 null symbol

  const size_t strtabOff = ehdrSize;
  const size_t symtabOff = strtabOff + strtab.size();
  const size_t shoff = symtabOff + symCount * symSize;
  const size_t totalSize = shoff + kSectionCount * shdrSize;

  std::vector<uint8_t> buf(totalSize, 0);

  auto *ehdr = reinterpret_cast<Elf64_Ehdr *>(buf.data());
  ehdr->e_ident[EI_MAG0] = ELFMAG0;
  ehdr->e_ident[EI_MAG1] = ELFMAG1;
  ehdr->e_ident[EI_MAG2] = ELFMAG2;
  ehdr->e_ident[EI_MAG3] = ELFMAG3;
  ehdr->e_ident[EI_CLASS] = ELFCLASS64;
  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_ident[EI_VERSION] = EV_CURRENT;
  ehdr->e_shoff = shoff;
  ehdr->e_shentsize = static_cast<Elf64_Half>(shdrSize);
  ehdr->e_shnum = kSectionCount;

  std::memcpy(buf.data() + strtabOff, strtab.data(), strtab.size());

  auto *syms = reinterpret_cast<Elf64_Sym *>(buf.data() + symtabOff);
  syms[0] = {};
  for (size_t i = 0; i < symbols.size(); ++i) {
    syms[i + 1].st_name = nameOffsets[i];
    syms[i + 1].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
  }

  auto *shdr = reinterpret_cast<Elf64_Shdr *>(buf.data() + shoff);
  shdr[0] = {};
  shdr[1].sh_type = SHT_STRTAB;
  shdr[1].sh_offset = strtabOff;
  shdr[1].sh_size = strtab.size();
  shdr[2].sh_type = SHT_DYNSYM;
  shdr[2].sh_offset = symtabOff;
  shdr[2].sh_size = symCount * symSize;
  shdr[2].sh_link = 1; // 指向 .dynstr
  shdr[2].sh_entsize = symSize;
  shdr[2].sh_addralign = 8;

  return buf;
}

// 将字节内容写入临时文件，析构时删除。
class TempFile {
public:
  explicit TempFile(const std::vector<uint8_t> &content) {
    std::string tmpl = "/tmp/elf_symbol_check_test_XXXXXX";
    std::vector<char> path(tmpl.begin(), tmpl.end());
    path.push_back('\0');
    int fd = mkstemp(path.data());
    if (fd < 0) {
      return;
    }
    path_ = path.data();
    if (!content.empty()) {
      auto written = write(fd, content.data(), content.size());
      (void)written;
    }
    close(fd);
  }

  ~TempFile() {
    if (!path_.empty()) {
      unlink(path_.c_str());
    }
  }

  const std::string &path() const { return path_; }

private:
  std::string path_;
};

// 构造 <tmpdir>/lib64/libruntime.so，导出 exportedSymbols
// 中的符号，析构时清理。
class TempToolkit {
public:
  explicit TempToolkit(const std::vector<std::string> &exportedSymbols) {
    std::string tmpl = "/tmp/elf_symbol_check_dir_XXXXXX";
    std::vector<char> path(tmpl.begin(), tmpl.end());
    path.push_back('\0');
    char *dir = mkdtemp(path.data());
    if (dir == nullptr) {
      return;
    }
    toolkitPath_ = dir;
    lib64Path_ = toolkitPath_ + "/lib64";
    soPath_ = lib64Path_ + "/libruntime.so";
    if (mkdir(lib64Path_.c_str(), 0755) != 0) {
      return;
    }
    auto content = BuildElfWithDynsyms(exportedSymbols);
    int fd = open(soPath_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
      return;
    }
    auto written = write(fd, content.data(), content.size());
    (void)written;
    close(fd);
    valid_ = true;
  }

  ~TempToolkit() {
    if (!toolkitPath_.empty()) {
      unlink(soPath_.c_str());
      rmdir(lib64Path_.c_str());
      rmdir(toolkitPath_.c_str());
    }
  }

  bool valid() const { return valid_; }
  const std::string &path() const { return toolkitPath_; }

private:
  bool valid_ = false;
  std::string toolkitPath_;
  std::string lib64Path_;
  std::string soPath_;
};

} // namespace

TEST(ElfSymbolCheckTest, HasSymbolInSoFindsExportedSymbol) {
  TempFile so(BuildElfWithDynsyms({"aclrtSetDeviceImpl"}));
  ASSERT_FALSE(so.path().empty());
  EXPECT_TRUE(HasSymbolInSo(so.path(), "aclrtSetDeviceImpl"));
  EXPECT_FALSE(HasSymbolInSo(so.path(), "notExportedSymbol"));
}

TEST(ElfSymbolCheckTest, HasSymbolInSoReturnsFalseWhenSymbolMissing) {
  TempFile so(BuildElfWithDynsyms({"someOtherSymbol"}));
  ASSERT_FALSE(so.path().empty());
  EXPECT_FALSE(HasSymbolInSo(so.path(), "aclrtSetDeviceImpl"));
}

TEST(ElfSymbolCheckTest, HasSymbolInSoReturnsFalseForEmptyDynsym) {
  TempFile so(BuildElfWithDynsyms({}));
  ASSERT_FALSE(so.path().empty());
  EXPECT_FALSE(HasSymbolInSo(so.path(), "aclrtSetDeviceImpl"));
}

TEST(ElfSymbolCheckTest, HasSymbolInSoReturnsFalseForMissingFile) {
  EXPECT_FALSE(HasSymbolInSo("/tmp/msdebug_so_not_exist_XXXXXX.so",
                             "aclrtSetDeviceImpl"));
}

TEST(ElfSymbolCheckTest, GetAclRuntimeLibNameReturnsRuntimeWhenSymbolPresent) {
  TempToolkit toolkit({"aclrtSetDeviceImpl"});
  ASSERT_TRUE(toolkit.valid());
  EXPECT_EQ("runtime", GetAclRuntimeLibName(toolkit.path()));
}

TEST(ElfSymbolCheckTest, GetAclRuntimeLibNameFallsBackWhenSymbolMissing) {
  TempToolkit toolkit({"someOtherSymbol"});
  ASSERT_TRUE(toolkit.valid());
  EXPECT_EQ("acl_rt_impl", GetAclRuntimeLibName(toolkit.path()));
}

TEST(ElfSymbolCheckTest, GetAclRuntimeLibNameFallsBackWhenFileMissing) {
  EXPECT_EQ("acl_rt_impl",
            GetAclRuntimeLibName("/tmp/msdebug_toolkit_not_exist_XXXXXX"));
}
