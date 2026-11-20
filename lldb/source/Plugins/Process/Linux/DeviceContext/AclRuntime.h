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
#ifndef liblldb_ACL_RUNTIME_H_
#define liblldb_ACL_RUNTIME_H_

#include "lldb/Utility/Status.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lldb_private {

// Thin, typed wrapper around the ACL runtime library loaded through dlopen.
//
// All ACL entry points are resolved into a private Functions struct. The
// struct is filled in a local instance and committed only after every symbol
// has been resolved, so a partially initialized AclRuntime is never exposed.
class AclRuntime {
public:
  enum class Direction : int32_t {
    HostToHost = 0,
    HostToDevice = 1,
    DeviceToHost = 2,
  };

  explicit AclRuntime(std::string library_name = "libacl_rt.so");
  ~AclRuntime();
  AclRuntime(const AclRuntime &) = delete;
  AclRuntime &operator=(const AclRuntime &) = delete;

  // dlopen the ACL runtime and resolve every required symbol.
  Status Initialize();
  Status AclInit();
  Status SetDevice(int32_t device_id);
  Status GetDevice(int32_t &device_id) const;
  // Make sure the calling context is bound to the expected device.
  Status EnsureDevice(int32_t device_id);
  Status Memcpy(void *dst, size_t dest_max, const void *src, size_t count,
                Direction dir);
  Status IpcMemImportByKey(void **shared_addr, const std::vector<char> &key);
  Status IpcMemClose(const std::vector<char> &key);

private:
  struct Functions {
    int (*init)(const char *) = nullptr;
    int (*get_device)(int *) = nullptr;
    int (*set_device)(int) = nullptr;
    int (*memcpy)(void *, size_t, const void *, size_t, int) = nullptr;
    int (*ipc_import)(void **, const char *, uint64_t) = nullptr;
    int (*ipc_close)(const char *) = nullptr;
    const char *(*get_recent_err_msg)() = nullptr;
  };

  using SymbolBinder = Status (*)(Functions &, void *);

  struct SymbolSpec {
    const char *name;
    SymbolBinder bind;
    bool required;
  };

  static const SymbolSpec kSymbols[];

  Status OpenAclLibrary();
  Status ResolveAll();
  bool IsReady() const {
    return m_functions.init && m_functions.get_device &&
           m_functions.set_device && m_functions.memcpy &&
           m_functions.ipc_import && m_functions.ipc_close;
  }

  std::string m_library_name;
  void *m_handle = nullptr;
  Functions m_functions;
};

} // namespace lldb_private

#endif // liblldb_ACL_RUNTIME_H_
#endif // MS_DEBUGGER
