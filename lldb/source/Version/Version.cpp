//===-- Version.cpp -------------------------------------------------------===//
//
// Modifications made to adapt for Ascend, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Version/Version.h"
#include "VCSVersion.inc"
#include "lldb/Version/Version.inc"
#include "clang/Basic/Version.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/llvm-config.h"

#ifdef MS_DEBUGGER
static const char *GetMsdebugPackageVersion() {
#ifdef MSDEBUG_VERSION_STRING
  return MSDEBUG_VERSION_STRING;
#else
  return nullptr;
#endif
}

static const char *GetMsdebugBuildDate() {
#ifdef MSDEBUG_BUILD_DATE
  return MSDEBUG_BUILD_DATE;
#else
  return nullptr;
#endif
}
#endif

static const char *GetLLDBVersion() {
#ifdef LLDB_FULL_VERSION_STRING
  return LLDB_FULL_VERSION_STRING;
#else
  return "lldb version " LLDB_VERSION_STRING;
#endif
}

static const char *GetLLDBRevision() {
#ifdef LLDB_REVISION
  return LLDB_REVISION;
#else
  return nullptr;
#endif
}

#ifndef MS_DEBUGGER
static const char *GetLLDBRepository() {
#ifdef LLDB_REPOSITORY
  return LLDB_REPOSITORY;
#else
  return nullptr;
#endif
}
#endif

#ifdef MS_DEBUGGER
static const char *GetMsdebugCommit() {
#ifdef MSDEBUG_GIT_COMMIT
  return MSDEBUG_GIT_COMMIT;
#else
  return GetLLDBRevision();
#endif
}

static bool IsUsable(llvm::StringRef value) {
  value = value.trim();
  return !value.empty() && !value.equals_insensitive("unknown") &&
         !value.equals_insensitive("n/a") &&
         !value.equals_insensitive("none");
}

static llvm::StringRef StripLLDBVersionPrefix(llvm::StringRef version) {
  version = version.trim();
  version.consume_front("lldb version ");
  return version.trim();
}

static void AppendDependency(std::string &dependencies, llvm::StringRef name,
                             llvm::StringRef version,
                             llvm::StringRef revision) {
  version = version.trim();
  revision = revision.trim();
  if (!IsUsable(version) || !IsUsable(revision))
    return;

  if (!dependencies.empty())
    dependencies += '\n';
  dependencies += "  " + name.str() + ": " + version.str() + " (" + revision.str() + ")";
}

const char *lldb_private::GetVersion() {
  static const std::string g_version_str = [] {
    const char *package_version = GetMsdebugPackageVersion();
    const char *commit = GetMsdebugCommit();
    const char *build_date = GetMsdebugBuildDate();

    const auto Or = [](const char *value) { return value ? value : "unknown"; };
    std::string version =
        "msdebug " + std::string(Or(package_version)) + " (" + Or(commit) +
        ")\nCopyright (c) 2026 Huawei Technologies Co., Ltd.\n"
        "License: Mulan PSL v2.\n\n"
        "Build Info:\n  Date: " + Or(build_date) +
        "\n  Repo: https://gitcode.com/Ascend/msdebug";

    std::string dependencies;
    AppendDependency(dependencies, "LLDB", StripLLDBVersionPrefix(GetLLDBVersion()),
                     GetLLDBRevision() ? GetLLDBRevision() : "");
    AppendDependency(dependencies, "Clang", CLANG_VERSION_STRING,
                     clang::getClangRevision());
    AppendDependency(dependencies, "LLVM", LLVM_VERSION_STRING,
                     clang::getLLVMRevision());
    if (!dependencies.empty()) {
      version += "\n\nDependencies:\n";
      version += dependencies;
    }
    return version;
  }();

  return g_version_str.c_str();
}
#else
const char *lldb_private::GetVersion() {
    static std::string g_version_str;

    if (g_version_str.empty()) {
      const char *lldb_version = GetLLDBVersion();
      const char *lldb_repo = GetLLDBRepository();
      const char *lldb_rev = GetLLDBRevision();
      g_version_str += lldb_version;
      if (lldb_repo || lldb_rev) {
        g_version_str += " (";
        if (lldb_repo)
          g_version_str += lldb_repo;
        if (lldb_repo && lldb_rev)
          g_version_str += " ";
        if (lldb_rev) {
          g_version_str += "revision ";
          g_version_str += lldb_rev;
        }
        g_version_str += ")";
      }

      std::string clang_rev(clang::getClangRevision());
      if (clang_rev.length() > 0) {
        g_version_str += "\n  clang revision ";
        g_version_str += clang_rev;
      }

      std::string llvm_rev(clang::getLLVMRevision());
      if (llvm_rev.length() > 0) {
        g_version_str += "\n  llvm revision ";
        g_version_str += llvm_rev;
      }
    }

    return g_version_str.c_str();
  }
#endif
