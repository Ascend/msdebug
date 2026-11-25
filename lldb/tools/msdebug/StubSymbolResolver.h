/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifdef MS_DEBUGGER
#ifndef MSDEBUG_STUB_SYMBOL_RESOLVER_H
#define MSDEBUG_STUB_SYMBOL_RESOLVER_H

#include "Singleton.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

// The three shared libraries the runtime stub resolves symbols from.
enum class StubLibrary {
  Runtime,
  AclRuntime,
  Driver,
};

// Single, thread-safe owner of the runtime-stub symbol tables.
//
// Both runtime_stub.cpp (rt* / drv* symbols) and aclrt_stub.cpp (aclrt*
// symbols) used to keep their own dlsym table plus their own one-shot init
// flag. This class merges them into one table and resolves each library
// lazily, so a symbol is only looked up in the library that actually exports
// it.
class StubSymbolResolver : public Singleton<StubSymbolResolver, false> {
public:
  StubSymbolResolver(const StubSymbolResolver &) = delete;
  StubSymbolResolver &operator=(const StubSymbolResolver &) = delete;

  // Resolve a symbol by name. When the symbol is missing and throwOnError is
  // true, ThrowErrorCode() is called with the symbol-specific error code.
  void *Resolve(const std::string &name, bool throwOnError = true);

  // True when an optional symbol has already been probed and found missing.
  bool IsKnownMissing(const std::string &name);

private:
  friend class Singleton<StubSymbolResolver, false>;

  StubSymbolResolver() = default;

  void EnsureInitializedLocked();
  void *ResolveLocked(const std::string &name, bool throwOnError);
  void *LibraryHandleLocked(StubLibrary library);
  void *OpenLibraryLocked(StubLibrary library, bool required);

  std::mutex m_mutex;
  bool m_initialized = false;
  void *m_rt_handle = nullptr;
  void *m_acl_handle = nullptr;
  void *m_drv_handle = nullptr;
  std::unordered_map<std::string, void *> m_symbols;
  std::unordered_set<std::string> m_missing;
};

#endif // MSDEBUG_STUB_SYMBOL_RESOLVER_H
#endif // MS_DEBUGGER
