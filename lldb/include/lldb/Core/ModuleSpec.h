//===-- ModuleSpec.h --------------------------------------------*- C++ -*-===//
//
// Modifications made to adapt for Ascend, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_CORE_MODULESPEC_H
#define LLDB_CORE_MODULESPEC_H

#include "lldb/Host/FileSystem.h"
#include "lldb/Target/PathMappingList.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/FileSpec.h"
#include "lldb/Utility/Iterable.h"
#include "lldb/Utility/Stream.h"
#include "lldb/Utility/UUID.h"

#include "llvm/Support/Chrono.h"
#ifdef MS_DEBUGGER
#include "lldb/lldb-enumerations.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/ErrorOr.h"
#endif

#include <mutex>
#include <vector>
#ifdef MS_DEBUGGER
#include <unistd.h>
#endif

namespace lldb_private {

class ModuleSpec {
public:
  ModuleSpec() = default;

  /// If the \c data argument is passed, its contents will be used
  /// as the module contents instead of trying to read them from
  /// \c file_spec .
  ModuleSpec(const FileSpec &file_spec, const UUID &uuid = UUID(),
             lldb::DataBufferSP data = lldb::DataBufferSP())
      : m_file(file_spec), m_uuid(uuid), m_object_offset(0), m_data(data) {
    if (data)
      m_object_size = data->GetByteSize();
    else if (m_file)
      m_object_size = FileSystem::Instance().GetByteSize(file_spec);
  }

  ModuleSpec(const FileSpec &file_spec, const ArchSpec &arch)
      : m_file(file_spec), m_arch(arch), m_object_offset(0),
        m_object_size(FileSystem::Instance().GetByteSize(file_spec)) {}

  FileSpec *GetFileSpecPtr() { return (m_file ? &m_file : nullptr); }

  const FileSpec *GetFileSpecPtr() const {
    return (m_file ? &m_file : nullptr);
  }

  FileSpec &GetFileSpec() { return m_file; }

  const FileSpec &GetFileSpec() const { return m_file; }

  FileSpec *GetPlatformFileSpecPtr() {
    return (m_platform_file ? &m_platform_file : nullptr);
  }

  const FileSpec *GetPlatformFileSpecPtr() const {
    return (m_platform_file ? &m_platform_file : nullptr);
  }

  FileSpec &GetPlatformFileSpec() { return m_platform_file; }

  const FileSpec &GetPlatformFileSpec() const { return m_platform_file; }

  FileSpec *GetSymbolFileSpecPtr() {
    return (m_symbol_file ? &m_symbol_file : nullptr);
  }

  const FileSpec *GetSymbolFileSpecPtr() const {
    return (m_symbol_file ? &m_symbol_file : nullptr);
  }

  FileSpec &GetSymbolFileSpec() { return m_symbol_file; }

  const FileSpec &GetSymbolFileSpec() const { return m_symbol_file; }

  ArchSpec *GetArchitecturePtr() {
    return (m_arch.IsValid() ? &m_arch : nullptr);
  }

  const ArchSpec *GetArchitecturePtr() const {
    return (m_arch.IsValid() ? &m_arch : nullptr);
  }

  ArchSpec &GetArchitecture() { return m_arch; }

  const ArchSpec &GetArchitecture() const { return m_arch; }

  UUID *GetUUIDPtr() { return (m_uuid.IsValid() ? &m_uuid : nullptr); }

  const UUID *GetUUIDPtr() const {
    return (m_uuid.IsValid() ? &m_uuid : nullptr);
  }

  UUID &GetUUID() { return m_uuid; }

  const UUID &GetUUID() const { return m_uuid; }

  ConstString &GetObjectName() { return m_object_name; }

  ConstString GetObjectName() const { return m_object_name; }

  uint64_t GetObjectOffset() const { return m_object_offset; }

  void SetObjectOffset(uint64_t object_offset) {
    m_object_offset = object_offset;
  }

  uint64_t GetObjectSize() const { return m_object_size; }

  void SetObjectSize(uint64_t object_size) { m_object_size = object_size; }

  llvm::sys::TimePoint<> &GetObjectModificationTime() {
    return m_object_mod_time;
  }

  const llvm::sys::TimePoint<> &GetObjectModificationTime() const {
    return m_object_mod_time;
  }

  PathMappingList &GetSourceMappingList() const { return m_source_mappings; }

  lldb::DataBufferSP GetData() const { return m_data; }

  void Clear() {
    m_file.Clear();
    m_platform_file.Clear();
    m_symbol_file.Clear();
    m_arch.Clear();
    m_uuid.Clear();
    m_object_name.Clear();
    m_object_offset = 0;
    m_object_size = 0;
    m_source_mappings.Clear(false);
    m_object_mod_time = llvm::sys::TimePoint<>();
  }

  explicit operator bool() const {
    if (m_file)
      return true;
    if (m_platform_file)
      return true;
    if (m_symbol_file)
      return true;
    if (m_arch.IsValid())
      return true;
    if (m_uuid.IsValid())
      return true;
    if (m_object_name)
      return true;
    if (m_object_size)
      return true;
    if (m_object_mod_time != llvm::sys::TimePoint<>())
      return true;
    return false;
  }

  void Dump(Stream &strm) const {
    bool dumped_something = false;
    if (m_file) {
      strm.PutCString("file = '");
      strm << m_file;
      strm.PutCString("'");
      dumped_something = true;
    }
    if (m_platform_file) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.PutCString("platform_file = '");
      strm << m_platform_file;
      strm.PutCString("'");
      dumped_something = true;
    }
    if (m_symbol_file) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.PutCString("symbol_file = '");
      strm << m_symbol_file;
      strm.PutCString("'");
      dumped_something = true;
    }
    if (m_arch.IsValid()) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.Printf("arch = ");
      m_arch.DumpTriple(strm.AsRawOstream());
      dumped_something = true;
    }
    if (m_uuid.IsValid()) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.PutCString("uuid = ");
      m_uuid.Dump(strm);
      dumped_something = true;
    }
    if (m_object_name) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.Printf("object_name = %s", m_object_name.GetCString());
      dumped_something = true;
    }
    if (m_object_offset > 0) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.Printf("object_offset = %" PRIu64, m_object_offset);
      dumped_something = true;
    }
    if (m_object_size > 0) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.Printf("object size = %" PRIu64, m_object_size);
      dumped_something = true;
    }
    if (m_object_mod_time != llvm::sys::TimePoint<>()) {
      if (dumped_something)
        strm.PutCString(", ");
      strm.Format("object_mod_time = {0:x+}",
                  uint64_t(llvm::sys::toTimeT(m_object_mod_time)));
    }
  }

  bool Matches(const ModuleSpec &match_module_spec,
               bool exact_arch_match) const {
    if (match_module_spec.GetUUIDPtr() &&
        match_module_spec.GetUUID() != GetUUID())
      return false;
    if (match_module_spec.GetObjectName() &&
        match_module_spec.GetObjectName() != GetObjectName())
      return false;
    if (!FileSpec::Match(match_module_spec.GetFileSpec(), GetFileSpec()))
      return false;
    if (GetPlatformFileSpec() &&
        !FileSpec::Match(match_module_spec.GetPlatformFileSpec(),
                         GetPlatformFileSpec())) {
      return false;
    }
    // Only match the symbol file spec if there is one in this ModuleSpec
    if (GetSymbolFileSpec() &&
        !FileSpec::Match(match_module_spec.GetSymbolFileSpec(),
                         GetSymbolFileSpec())) {
      return false;
    }
    if (match_module_spec.GetArchitecturePtr()) {
      if (exact_arch_match) {
        if (!GetArchitecture().IsExactMatch(
                match_module_spec.GetArchitecture()))
          return false;
      } else {
        if (!GetArchitecture().IsCompatibleMatch(
                match_module_spec.GetArchitecture()))
          return false;
      }
    }
    return true;
  }

#ifdef MS_DEBUGGER
  Status CheckOwnerAndWritablePermission() {
    Status error;
    FileSpec &file = GetFileSpec();
    llvm::ErrorOr<llvm::vfs::Status> status = FileSystem::Instance().GetStatus(file);
    const uint32_t permissions = FileSystem::Instance().GetPermissions(file);

    if (status) {
      // 不允许其他用户组可写
      if (permissions & lldb::eFilePermissionsWorldWrite) {
        error.SetErrorStringWithFormat("Risky action, \"%s\" is writable by any other users.",
                                       file.GetPath().c_str());
        return error;
      }
      // 目标文件是root所有的，可以被任意用户执行；目标文件非root所有，只能被实际所有者执行
      // 当前用户为root时, 不做文件从属权限校验
      if ((getuid() != 0) && (status->getUser() != 0 && status->getUser() != getuid())) {
        error.SetErrorStringWithFormat("Risky action, \"%s\" is not owned by root or current user.",
                                       file.GetPath().c_str());
        return error;
      }
    } else {
      error.SetErrorStringWithFormat(
        "Unable to find \"%s\". File does Not exist.",
        file.GetPath().c_str());
    }
    return error;
  }

  Status CheckExecutablePermission() {
    Status error;
    FileSpec &file = GetFileSpec();
    llvm::ErrorOr<llvm::vfs::Status> status = FileSystem::Instance().GetStatus(file);

    if(!status) {
      error.SetErrorStringWithFormat(
        "Unable to find \"%s\". File does Not exist.",
        file.GetPath().c_str());
      return error;
    }

    const uint32_t file_permissions = FileSystem::Instance().GetPermissions(file);
    const uint32_t curr_user = getuid();
    const uint32_t curr_group = getgid();
    const uint32_t curr_file_owner = status->getUser();
    const uint32_t curr_file_group = status->getGroup();

    // 0 represents the root's uid.
    if (curr_user != 0) {
      if ((curr_file_owner == curr_user) &&
          (file_permissions & lldb::eFilePermissionsUserExecute)) {
        return error;
      } else if ((curr_file_group == curr_group) &&
                 (file_permissions & lldb::eFilePermissionsGroupExecute)) {
        return error;
      } else if (file_permissions & lldb::eFilePermissionsWorldExecute) {
        return error;
      }
      error.SetErrorStringWithFormat("Missing executable permissions for \"%s\".",
        file.GetPath().c_str());
    } else if ((file_permissions & lldb::eFilePermissionsEveryoneX) == 0) {
      error.SetErrorStringWithFormat(
        "Missing executable permissions for \"%s\".",
        file.GetPath().c_str());
    }
    return error;
  }

  Status CheckReadablePermission() {
    Status error;
    FileSpec &file = GetFileSpec();
    llvm::ErrorOr<llvm::vfs::Status> status = FileSystem::Instance().GetStatus(file);

    if(!status) {
      error.SetErrorStringWithFormat(
              "Unable to find \"%s\". File does Not exist.",
              file.GetPath().c_str());
      return error;
    }

    const uint32_t file_permissions = FileSystem::Instance().GetPermissions(file);
    const uint32_t curr_user = getuid();
    const uint32_t curr_group = getgid();
    const uint32_t curr_file_owner = status->getUser();
    const uint32_t curr_file_group = status->getGroup();

    // 0 represents the root's uid.
    if (curr_user != 0) {
      if ((curr_file_owner == curr_user) &&
          (file_permissions & lldb::eFilePermissionsUserRead)) {
        return error;
      } else if ((curr_file_group == curr_group) &&
                 (file_permissions & lldb::eFilePermissionsGroupRead)) {
        return error;
      } else if (file_permissions & lldb::eFilePermissionsWorldRead) {
        return error;
      }
      error.SetErrorStringWithFormat("Missing readable permissions for \"%s\".",
                                     file.GetPath().c_str());
    } else if ((file_permissions & lldb::eFilePermissionsEveryoneR) == 0) {
      error.SetErrorStringWithFormat(
              "Missing readable permissions for \"%s\".",
              file.GetPath().c_str());
    }
    return error;
  }

  // Symbolic link = symlink = soft link
  Status CheckIsSymlink() {
    Status error;
    FileSpec &file = GetFileSpec();
    std::string path = file.GetPath();

    if(FileSystem::Instance().IsSymlink(path)){
      error.SetErrorStringWithFormat("\"%s\" is a soft link, which is not supported.", path.c_str());
    }
    return error;
  }

  Status CheckPathExists(){
    Status error;
    FileSpec &file = GetFileSpec();

    if(!FileSystem::Instance().Exists(file)){
      error.SetErrorStringWithFormat("\"%s\" does NOT exist, please check it.", file.GetPath().c_str());
    }
    return error;
  }

  Status CheckIsDir() {
    Status error;
    FileSpec &file = GetFileSpec();

    if(FileSystem::Instance().IsDirectory(file)){
      error.SetErrorStringWithFormat("\"%s\" is a directory, which is not supported.", file.GetPath().c_str());
    }
    return error;
  }

  Status CheckFileSize() {
    Status error;
    FileSpec &file = GetFileSpec();
    auto size = FileSystem::Instance().GetByteSize(file);
    constexpr uint64_t MAX_FILE_SIZE = 100ULL * 1024 * 1024 * 1024;
    if (size == 0) {
      error.SetErrorStringWithFormatv("\"{0}\" is an empty file, which is not supported.", file.GetPath());
    } else if (size > MAX_FILE_SIZE) {
      error.SetErrorStringWithFormatv("\"{0}\" is too large, total {1} bytes, limit is {2} bytes.",
                                      file.GetPath(), size, MAX_FILE_SIZE);
    }
    return error;
  }

  Status CheckParentPathOwnerAndWritablePermission() {
    Status error;
    FileSpec parentFile(GetFileSpec().GetDirectory().GetStringRef());
    llvm::ErrorOr<llvm::vfs::Status> status = FileSystem::Instance().GetStatus(parentFile);
    const uint32_t permissions = FileSystem::Instance().GetPermissions(parentFile);

    if (status) {
      // 目标文件是root所有的，可以被任意用户执行；目标文件非root所有，只能被实际所有者执行
      // 当前用户为root时, 不做文件从属权限校验
      if ((getuid() != 0) && (status->getUser() != 0 && status->getUser() != getuid())) {
        error.SetErrorStringWithFormat("Risky action, \"%s\" is not owned by root or current user.",
                                       parentFile.GetPath().c_str());
        return error;
      }

      // 不允许其他或用户组可写
      if (permissions & (lldb::eFilePermissionsWorldWrite | lldb::eFilePermissionsGroupWrite)) {
        error.SetErrorStringWithFormat("Risky action, \"%s\" is writable by any other users or groups.",
                                       parentFile.GetPath().c_str());
        return error;
      }
    } else {
      error.SetErrorStringWithFormat(
          "Unable to find \"%s\". File does Not exist.",
          parentFile.GetPath().c_str());
    }
    return error;
  }

  Status CheckInputFileValid(CheckInputValidClass flags) {
    Status error;
    // when add a new term, it should follow the check-valid order;
    // keep the same order as lldb/include/lldb/lldb-private-enumerations.h CheckInputValidClass
    std::vector<std::function<Status(ModuleSpec*)>> checker_register_v = {
      &ModuleSpec::CheckPathExists,
      &ModuleSpec::CheckOwnerAndWritablePermission,
      &ModuleSpec::CheckIsSymlink,
      &ModuleSpec::CheckReadablePermission,
      &ModuleSpec::CheckIsDir,
      &ModuleSpec::CheckFileSize,
      &ModuleSpec::CheckExecutablePermission,
      &ModuleSpec::CheckParentPathOwnerAndWritablePermission
    };

    auto execute = [this, &error, flags](std::vector<std::function<Status(ModuleSpec*)>>& vc) mutable {
      for (unsigned int i = 0; i < vc.size(); ++i){
        if((static_cast<unsigned int>(flags)) & (1 << i)){
          error = vc[i](this);
          if (error.Fail())
            return;
        }
      }
    };
    execute(checker_register_v);
    return error;
  }

#endif

protected:
  FileSpec m_file;
  FileSpec m_platform_file;
  FileSpec m_symbol_file;
  ArchSpec m_arch;
  UUID m_uuid;
  ConstString m_object_name;
  uint64_t m_object_offset = 0;
  uint64_t m_object_size = 0;
  llvm::sys::TimePoint<> m_object_mod_time;
  mutable PathMappingList m_source_mappings;
  lldb::DataBufferSP m_data = {};
};

class ModuleSpecList {
public:
  ModuleSpecList() = default;

  ModuleSpecList(const ModuleSpecList &rhs) {
    std::lock_guard<std::recursive_mutex> lhs_guard(m_mutex);
    std::lock_guard<std::recursive_mutex> rhs_guard(rhs.m_mutex);
    m_specs = rhs.m_specs;
  }

  ~ModuleSpecList() = default;

  ModuleSpecList &operator=(const ModuleSpecList &rhs) {
    if (this != &rhs) {
      std::lock(m_mutex, rhs.m_mutex);
      std::lock_guard<std::recursive_mutex> lhs_guard(m_mutex, std::adopt_lock);
      std::lock_guard<std::recursive_mutex> rhs_guard(rhs.m_mutex,
                                                      std::adopt_lock);
      m_specs = rhs.m_specs;
    }
    return *this;
  }

  size_t GetSize() const {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    return m_specs.size();
  }

  void Clear() {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_specs.clear();
  }

  void Append(const ModuleSpec &spec) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_specs.push_back(spec);
  }

  void Append(const ModuleSpecList &rhs) {
    std::lock_guard<std::recursive_mutex> lhs_guard(m_mutex);
    std::lock_guard<std::recursive_mutex> rhs_guard(rhs.m_mutex);
    m_specs.insert(m_specs.end(), rhs.m_specs.begin(), rhs.m_specs.end());
  }

  // The index "i" must be valid and this can't be used in multi-threaded code
  // as no mutex lock is taken.
  ModuleSpec &GetModuleSpecRefAtIndex(size_t i) { return m_specs[i]; }

  bool GetModuleSpecAtIndex(size_t i, ModuleSpec &module_spec) const {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (i < m_specs.size()) {
      module_spec = m_specs[i];
      return true;
    }
    module_spec.Clear();
    return false;
  }

  bool FindMatchingModuleSpec(const ModuleSpec &module_spec,
                              ModuleSpec &match_module_spec) const {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    bool exact_arch_match = true;
    for (auto spec : m_specs) {
      if (spec.Matches(module_spec, exact_arch_match)) {
        match_module_spec = spec;
        return true;
      }
    }

    // If there was an architecture, retry with a compatible arch
    if (module_spec.GetArchitecturePtr()) {
      exact_arch_match = false;
      for (auto spec : m_specs) {
        if (spec.Matches(module_spec, exact_arch_match)) {
          match_module_spec = spec;
          return true;
        }
      }
    }
    match_module_spec.Clear();
    return false;
  }

  void FindMatchingModuleSpecs(const ModuleSpec &module_spec,
                               ModuleSpecList &matching_list) const {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    bool exact_arch_match = true;
    const size_t initial_match_count = matching_list.GetSize();
    for (auto spec : m_specs) {
      if (spec.Matches(module_spec, exact_arch_match))
        matching_list.Append(spec);
    }

    // If there was an architecture, retry with a compatible arch if no matches
    // were found
    if (module_spec.GetArchitecturePtr() &&
        (initial_match_count == matching_list.GetSize())) {
      exact_arch_match = false;
      for (auto spec : m_specs) {
        if (spec.Matches(module_spec, exact_arch_match))
          matching_list.Append(spec);
      }
    }
  }

  void Dump(Stream &strm) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    uint32_t idx = 0;
    for (auto spec : m_specs) {
      strm.Printf("[%u] ", idx);
      spec.Dump(strm);
      strm.EOL();
      ++idx;
    }
  }

  typedef std::vector<ModuleSpec> collection;
  typedef LockingAdaptedIterable<collection, ModuleSpec, vector_adapter,
                                 std::recursive_mutex>
      ModuleSpecIterable;

  ModuleSpecIterable ModuleSpecs() {
    return ModuleSpecIterable(m_specs, m_mutex);
  }

protected:
  collection m_specs;                         ///< The collection of modules.
  mutable std::recursive_mutex m_mutex;
};

} // namespace lldb_private

#endif // LLDB_CORE_MODULESPEC_H
