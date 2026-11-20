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
#include "AclRuntime.h"

#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"

#include <dlfcn.h>
#include <utility>

using namespace lldb_private;

namespace {

constexpr uint64_t kIpcMemImportFlagPeerAccess = 0x1UL;

template <typename Fn, typename... Args>
Status CallAcl(Fn fn, const char *name, Args &&...args) {
  if (fn == nullptr) {
    Status error;
    error.SetErrorStringWithFormatv("{0} is not available", name);
    return error;
  }
  const int32_t ret = fn(std::forward<Args>(args)...);
  if (ret != 0) {
    LLDB_LOGF(GetLog(LLDBLog::Process), "%s failed ret=%d", name, ret);
    Status error;
    error.SetErrorStringWithFormatv("{0} failed ret={1}", name, ret);
    return error;
  }
  return Status();
}

} // namespace

// Only public ACL entry points are resolved. The implementation-only *Impl
// names are intentionally not used: the server always loads libacl_rt.so,
// which exports the public symbols.
const AclRuntime::SymbolSpec AclRuntime::kSymbols[] = {
    {"aclInit",
     [](Functions &functions, void *symbol) {
       functions.init = reinterpret_cast<decltype(functions.init)>(symbol);
       return Status();
     },
     true},
    {"aclrtGetDevice",
     [](Functions &functions, void *symbol) {
       functions.get_device =
           reinterpret_cast<decltype(functions.get_device)>(symbol);
       return Status();
     },
     true},
    {"aclrtSetDevice",
     [](Functions &functions, void *symbol) {
       functions.set_device =
           reinterpret_cast<decltype(functions.set_device)>(symbol);
       return Status();
     },
     true},
    {"aclrtMemcpy",
     [](Functions &functions, void *symbol) {
       functions.memcpy = reinterpret_cast<decltype(functions.memcpy)>(symbol);
       return Status();
     },
     true},
    {"aclrtIpcMemImportByKey",
     [](Functions &functions, void *symbol) {
       functions.ipc_import =
           reinterpret_cast<decltype(functions.ipc_import)>(symbol);
       return Status();
     },
     true},
    {"aclrtIpcMemClose",
     [](Functions &functions, void *symbol) {
       functions.ipc_close =
           reinterpret_cast<decltype(functions.ipc_close)>(symbol);
       return Status();
     },
     true},
    {"aclGetRecentErrMsg",
     [](Functions &functions, void *symbol) {
       functions.get_recent_err_msg =
           reinterpret_cast<decltype(functions.get_recent_err_msg)>(symbol);
       return Status();
     },
     false},
};

AclRuntime::AclRuntime(std::string library_name)
    : m_library_name(std::move(library_name)) {}

AclRuntime::~AclRuntime() {
  if (m_handle != nullptr) {
    dlclose(m_handle);
    m_handle = nullptr;
  }
}

Status AclRuntime::OpenAclLibrary() {
  if (m_handle != nullptr) {
    return Status();
  }

  m_handle = dlopen(m_library_name.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (m_handle == nullptr) {
    const char *reason = dlerror();
    Status error;
    error.SetErrorStringWithFormatv("dlopen {0} failed: {1}", m_library_name,
                                    reason != nullptr ? reason
                                                      : "unknown error");
    return error;
  }
  return Status();
}

Status AclRuntime::ResolveAll() {
  Log *log = GetLog(LLDBLog::Process);
  Functions functions;
  for (const auto &spec : kSymbols) {
    void *symbol = dlsym(m_handle, spec.name);
    if (symbol == nullptr) {
      const char *reason = dlerror();
      if (!spec.required) {
        LLDB_LOG(log, "optional dlsym {0} failed: {1}", spec.name,
                 reason != nullptr ? reason : "unknown error");
        continue;
      }
      LLDB_LOG(log, "dlsym {0} failed: {1}", spec.name,
               reason != nullptr ? reason : "unknown error");
      Status error;
      error.SetErrorStringWithFormatv("dlsym {0} failed: {1}", spec.name,
                                      reason != nullptr ? reason
                                                        : "unknown error");
      return error;
    }
    Status error = spec.bind(functions, symbol);
    if (error.Fail()) {
      LLDB_LOG(log, "{0}", error);
      return error;
    }
  }

  // Commit only after every required symbol has been resolved.
  m_functions = functions;
  return Status();
}

Status AclRuntime::Initialize() {
  Status error = OpenAclLibrary();
  if (error.Fail()) {
    return error;
  }
  return ResolveAll();
}

Status AclRuntime::AclInit() {
  if (!IsReady()) {
    return Status("AclRuntime has not been initialized");
  }
  Status error = CallAcl(m_functions.init, "aclInit", nullptr);
  if (error.Fail()) {
    return error;
  }
  LLDB_LOG(GetLog(LLDBLog::Process), "aclInit done");
  return Status();
}

Status AclRuntime::SetDevice(int32_t device_id) {
  Status error = CallAcl(m_functions.set_device, "aclrtSetDevice", device_id);
  if (error.Fail()) {
    LLDB_LOGF(GetLog(LLDBLog::Process), "aclrtSetDevice device_id=%d",
              device_id);
  }
  return error;
}

Status AclRuntime::GetDevice(int32_t &device_id) const {
  return CallAcl(m_functions.get_device, "aclrtGetDevice", &device_id);
}

Status AclRuntime::EnsureDevice(int32_t device_id) {
  Log *log = GetLog(LLDBLog::Process);
  int32_t current_device_id = -1;
  Status error = GetDevice(current_device_id);
  if (error.Success() && current_device_id == device_id) {
    return Status();
  }
  LLDB_LOGF(log,
            "aclrtGetDevice current_device_id=%d target_device_id=%d, retry "
            "setDevice",
            current_device_id, device_id);
  return SetDevice(device_id);
}

Status AclRuntime::Memcpy(void *dst, size_t dest_max, const void *src,
                          size_t count, Direction dir) {
  return CallAcl(m_functions.memcpy, "aclrtMemcpy", dst, dest_max, src, count,
                 static_cast<int32_t>(dir));
}

Status AclRuntime::IpcMemImportByKey(void **shared_addr,
                                     const std::vector<char> &key) {
  Log *log = GetLog(LLDBLog::Process);
  Status error = CallAcl(m_functions.ipc_import, "aclrtIpcMemImportByKey",
                         shared_addr, key.data(), kIpcMemImportFlagPeerAccess);
  if (error.Fail()) {
    if (m_functions.get_recent_err_msg != nullptr) {
      const char *recent = m_functions.get_recent_err_msg();
      if (recent != nullptr && recent[0] != '\0') {
        LLDB_LOGF(log, "aclrtIpcMemImportByKey recent error: %s", recent);
        Status with_recent;
        with_recent.SetErrorStringWithFormatv(
            "{0}; recent: {1}",
            error.AsCString() != nullptr ? error.AsCString() : "unknown error",
            recent);
        return with_recent;
      }
    }
    return error;
  }
  LLDB_LOG(log, "aclrtIpcMemImportByKey done");
  return Status();
}

Status AclRuntime::IpcMemClose(const std::vector<char> &key) {
  Status error = CallAcl(m_functions.ipc_close, "aclrtIpcMemClose", key.data());
  if (error.Fail()) {
    return error;
  }
  LLDB_LOG(GetLog(LLDBLog::Process), "aclrtIpcMemClose done");
  return Status();
}

#endif // MS_DEBUGGER
