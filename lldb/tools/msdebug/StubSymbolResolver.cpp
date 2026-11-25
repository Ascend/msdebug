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
#include "StubSymbolResolver.h"

#include "elf_symbol_check.h"
#include "rt_stub_log.h"

#include <cstdlib>
#include <dlfcn.h>

// Defined in runtime_stub.cpp. Kept as a free function because it is also
// declared in runtime_stub.h and exercised by the smoke test.
void EnvCheck();

namespace {

struct SymbolSpec {
  const char *name;
  StubLibrary library;
  MSDEBUG_ERROR_CODE missing_error;
};

// The single source of truth for every symbol the runtime stub resolves.
// Adding a new hijacked dependency is one table row.
constexpr SymbolSpec kSymbolSpecs[] = {
    // rt* symbols
    {"rtSetDevice", StubLibrary::Runtime, RT_SET_DEVICE_SYM_NOT_FOUND_ERR},
    {"rtSetDeviceEx", StubLibrary::Runtime, RT_SET_DEVICE_EX_SYM_NOT_FOUND_ERR},
    {"rtCtxCreateEx", StubLibrary::Runtime, RT_CTX_CREATE_EX_SYM_NOT_FOUND_ERR},
    {"rtFunctionRegister", StubLibrary::Runtime, RT_FUNC_REG_NOT_FOUND_ERR},
    {"rtKernelLaunch", StubLibrary::Runtime, RT_KERNEL_LAUNCH_NOT_FOUND_ERR},
    {"rtKernelLaunchWithFlagV2", StubLibrary::Runtime,
     RT_KERNEL_LAUNCH_WITH_FLAG_V2_NOT_FOUND_ERR},
    {"rtKernelLaunchWithHandleV2", StubLibrary::Runtime,
     RT_KERNEL_LAUNCH_WITH_HANDLE_V2_NOT_FOUND_ERR},
    {"rtKernelGetAddrAndPrefCnt", StubLibrary::Runtime,
     RT_KERNEL_GET_ADDR_AND_PREF_CNT_NOT_FOUND_ERR},
    {"rtGetSocVersion", StubLibrary::Runtime, RT_GET_SOC_VERSION_NOT_FOUND_ERR},
    {"rtDevBinaryRegister", StubLibrary::Runtime, RT_DEV_BIN_REG_NOT_FOUND_ERR},
    {"rtRegisterAllKernel", StubLibrary::Runtime,
     RT_REG_ALL_KERNEL_NOT_FOUND_ERR},
    {"rtGetStreamId", StubLibrary::Runtime,
     RT_GET_TASK_ID_AND_STREAM_ID_NOT_FOUND_ERR},
    {"rtStreamSynchronizeWithTimeout", StubLibrary::Runtime,
     RT_STREAM_SYNC_WITH_TIMEOUT_NOT_FOUND_ERR},
    {"rtStreamSynchronize", StubLibrary::Runtime, RT_STREAM_SYNC_NOT_FOUND_ERR},
    {"rtRegisterFuncSymbol", StubLibrary::Runtime,
     RT_REGISTER_FUNC_SYMBOL_NOT_FOUND_ERR},
    {"rtGetVisibleDeviceIdByLogicDeviceId", StubLibrary::Runtime,
     RT_GET_VISIBLE_DEVID_BY_LOGIC_DEVID_NOT_FOUND_ERR},
    {"rtMemGetAddressRange", StubLibrary::Runtime,
     RT_MEM_GET_ADDRESS_RANGE_NOT_FOUND_ERR},
    {"rtGetDevice", StubLibrary::Runtime, RT_GET_DEVICE_NOT_FOUND_ERR},

    // aclrt* symbols
    {"aclrtGetDeviceImpl", StubLibrary::AclRuntime,
     ACLRT_GET_DEVICE_IMPL_NOT_FOUND_ERR},
    {"aclrtSetDeviceImpl", StubLibrary::AclRuntime,
     ACLRT_SET_DEVICE_IMPL_NOT_FOUND_ERR},
    {"aclrtCreateContextImpl", StubLibrary::AclRuntime,
     ACLRT_CREATE_CONTEXT_IMPL_NOT_FOUND_ERR},
    {"aclrtBinaryGetFunctionImpl", StubLibrary::AclRuntime,
     ACLRT_BINARY_GET_FUNCTION_IMPL_NOT_FOUND_ERR},
    {"aclrtBinaryGetFunctionByEntryImpl", StubLibrary::AclRuntime,
     ACLRT_BINARY_GET_FUNCTION_BY_ENTRY_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchKernelImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_KERNEL_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchKernelWithConfigImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_KERNEL_WITH_CONFIG_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchKernelWithHostArgsImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_KERNEL_WITH_HOST_ARGS_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchKernelV2Impl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_KERNEL_V2_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchSIMTKernelWithArgsArrayImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_SIMT_KERNEL_WITH_ARGS_ARRAY_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchKernelWithArgsArrayImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_KERNEL_WITH_ARGS_ARRAY_IMPL_NOT_FOUND_ERR},
    {"aclrtLaunchSIMTKernelWithHostArgsImpl", StubLibrary::AclRuntime,
     ACLRT_LAUNCH_SIMT_KERNEL_WITH_HOST_ARGS_IMPL_NOT_FOUND_ERR},
    {"aclrtGetFunctionAddrImpl", StubLibrary::AclRuntime,
     ACLRT_GET_FUNCTION_ADDR_IMPL_NOT_FOUND_ERR},
    {"aclrtGetSocNameImpl", StubLibrary::AclRuntime,
     ACLRT_GET_SOC_NAME_IMPL_NOT_FOUND_ERR},
    {"aclrtBinaryLoadFromFileImpl", StubLibrary::AclRuntime,
     ACLRT_BINARY_LOAD_FROM_FILE_IMPL_NOT_FOUND_ERR},
    {"aclrtCreateBinaryImpl", StubLibrary::AclRuntime,
     ACLRT_CREATE_BINARY_IMPL_NOT_FOUND_ERR},
    {"aclrtBinaryLoadImpl", StubLibrary::AclRuntime,
     ACLRT_BINARY_LOAD_IMPL_NOT_FOUND_ERR},
    {"aclrtBinaryLoadFromDataImpl", StubLibrary::AclRuntime,
     ACLRT_BINARY_LOAD_FROM_DATA_IMPL_NOT_FOUND_ERR},
    {"aclrtStreamGetIdImpl", StubLibrary::AclRuntime,
     ACLRT_STREAM_GET_ID_IMPL_NOT_FOUND_ERR},
    {"aclrtSynchronizeStreamImpl", StubLibrary::AclRuntime,
     ACLRT_SYNC_STREAM_IMPL_NOT_FOUND_ERR},
    {"aclrtMemGetAddressRangeImpl", StubLibrary::AclRuntime,
     ACLRT_MEM_GET_ADDRESS_RANGE_IMPL_NOT_FOUND_ERR},
    {"aclrtGetLogicDevIdByUserDevIdImpl", StubLibrary::AclRuntime,
     ACLRT_GET_LOGIC_DEVID_BY_USER_DEVID_IMPL_NOT_FOUND_ERR},
    {"aclrtFunctionGetBinaryImpl", StubLibrary::AclRuntime,
     ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR},
    {"aclrtGetFunctionNameImpl", StubLibrary::AclRuntime,
     ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR},
    {"aclrtGetFuncBySymbolImpl", StubLibrary::AclRuntime,
     ACLRT_GET_FUNC_BY_SYMBOL_IMPL_NOT_FOUND_ERR},
    {"aclrtSynchronizeDeviceWithTimeoutImpl", StubLibrary::AclRuntime,
     ACLRT_SYNCHRONIZE_DEVICE_WITH_TIMEOUT_NOT_FOUND_ERR},
    {"aclrtSynchronizeStreamWithTimeoutImpl", StubLibrary::AclRuntime,
     ACLRT_SYNC_STREAM_WITH_TIMEOUT_IMPL_NOT_FOUND_ERR},
    {"aclrtIpcMemGetExportKeyImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtIpcMemCloseImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtMallocImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtMallocAlign32Impl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtMallocCachedImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtMallocWithCfgImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtFreeImpl", StubLibrary::AclRuntime, ACCESS_INVALID_RT_FUNC_NAME_ERR},
    {"aclrtFreeWithDevSyncImpl", StubLibrary::AclRuntime,
     ACCESS_INVALID_RT_FUNC_NAME_ERR},

    // driver / hal symbols
    {"drvDeviceGetBareTgid", StubLibrary::Driver,
     DRV_DEV_GET_BARE_TGID_NOT_FOUND_ERR},
    {"halMemAdvise", StubLibrary::Driver, HAL_MEM_ADVISE_NOT_FOUND_ERR},
};

const SymbolSpec *FindSpec(const std::string &name) {
  for (const auto &spec : kSymbolSpecs) {
    if (name == spec.name) {
      return &spec;
    }
  }
  return nullptr;
}

std::string GetToolkitPath() {
  char *toolkitPath = getenv("ASCEND_TOOLKIT_HOME");
  if (toolkitPath == nullptr) {
    ThrowErrorCode(ASCEND_TOOLKIT_HOME_NOT_FOUND_ERR);
    return std::string();
  }
  return std::string(toolkitPath);
}

} // namespace

void StubSymbolResolver::EnsureInitializedLocked() {
  if (m_initialized) {
    return;
  }
  try {
    RtStubLogger::Instance().Init();
  } catch (...) {
    RT_STUB_LOG_ERROR("Invalid value of $DEBUGGER_RT_STUB_LOG\n");
    ThrowErrorCode(ENV_VALUE_TYPE_ERR);
  }
  RT_STUB_LOG_INFO("LogInit done\n");
  // Keep the historical runtime/aclrt-before-driver order; dlsym stays lazy.
  OpenLibraryLocked(StubLibrary::Runtime, false);
  OpenLibraryLocked(StubLibrary::AclRuntime, false);
  EnvCheck();
  m_initialized = true;
  RT_STUB_LOG_INFO("StubInit done\n");
}

void *StubSymbolResolver::LibraryHandleLocked(StubLibrary library) {
  return OpenLibraryLocked(library, true);
}

void *StubSymbolResolver::OpenLibraryLocked(StubLibrary library,
                                            bool required) {
  switch (library) {
  case StubLibrary::Runtime: {
    if (m_rt_handle == nullptr) {
      std::string runtime_lib_path = GetToolkitPath() + "/lib64/libruntime.so";
      m_rt_handle = dlopen(runtime_lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
      if (m_rt_handle == nullptr) {
        RT_STUB_LOG_WARNING("dlopen %s failed\n", runtime_lib_path.c_str());
        if (required) {
          ThrowErrorCode(LIB_RUNTIME_NOT_FOUND_ERR);
        }
        return nullptr;
      }
      RT_STUB_LOG_INFO("dlopen %s done\n", runtime_lib_path.c_str());
    }
    return m_rt_handle;
  }
  case StubLibrary::AclRuntime: {
    if (m_acl_handle == nullptr) {
      std::string toolkit_path = GetToolkitPath();
      std::string so_name = GetAclRuntimeLibName(toolkit_path);
      std::string runtime_lib_path =
          toolkit_path + "/lib64/lib" + so_name + ".so";
      m_acl_handle = dlopen(runtime_lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
      if (m_acl_handle == nullptr) {
        std::string old_acl_impl_path =
            toolkit_path + "/lib64/libascendcl_impl.so";
        RT_STUB_LOG_INFO("dlopen %s failed, change to open %s\n",
                         runtime_lib_path.c_str(), old_acl_impl_path.c_str());
        m_acl_handle =
            dlopen(old_acl_impl_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (m_acl_handle == nullptr) {
          RT_STUB_LOG_WARNING("dlopen %s failed\n", old_acl_impl_path.c_str());
          if (required) {
            ThrowErrorCode(LIB_ACL_RUNTIME_IMPL_NOT_FOUND_ERR);
          }
          return nullptr;
        }
      }
      RT_STUB_LOG_INFO("dlopen aclrt impl done\n");
    }
    return m_acl_handle;
  }
  case StubLibrary::Driver: {
    if (m_drv_handle == nullptr) {
      m_drv_handle = dlopen("libascend_hal.so", RTLD_NOW | RTLD_GLOBAL);
      if (m_drv_handle == nullptr) {
        RT_STUB_LOG_WARNING("dlopen libascend_hal.so failed\n");
        if (required) {
          ThrowErrorCode(LIB_ASCEND_HAL_NOT_FOUND_ERR);
        }
        return nullptr;
      }
      RT_STUB_LOG_INFO("dlopen hal done\n");
    }
    return m_drv_handle;
  }
  }
  return nullptr;
}

void *StubSymbolResolver::ResolveLocked(const std::string &name,
                                        bool throwOnError) {
  auto cached = m_symbols.find(name);
  if (cached != m_symbols.end()) {
    return cached->second;
  }
  if (m_missing.count(name) != 0) {
    if (throwOnError) {
      const SymbolSpec *missing_spec = FindSpec(name);
      ThrowErrorCode(missing_spec != nullptr ? missing_spec->missing_error
                                             : ACCESS_INVALID_RT_FUNC_NAME_ERR);
    }
    return nullptr;
  }

  const SymbolSpec *spec = FindSpec(name);
  if (spec == nullptr) {
    if (throwOnError) {
      ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
    }
    return nullptr;
  }

  void *handle = LibraryHandleLocked(spec->library);
  void *symbol = dlsym(handle, name.c_str());
  if (symbol == nullptr) {
    if (throwOnError) {
      RT_STUB_LOG_WARNING("dlsym %s failed\n", name.c_str());
      ThrowErrorCode(spec->missing_error);
    }
    RT_STUB_LOG_WARNING("optional dlsym %s failed, cached as missing\n",
                        name.c_str());
    m_missing.insert(name);
    return nullptr;
  }

  m_symbols.emplace(name, symbol);
  return symbol;
}

bool StubSymbolResolver::IsKnownMissing(const std::string &name) {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_missing.count(name) != 0;
}

void *StubSymbolResolver::Resolve(const std::string &name, bool throwOnError) {
  RT_STUB_LOG_INFO("GetStubFuncPtr funcName=%s\n", name.c_str());
  // Keep the historical behavior: an unknown name fails before any lazy
  // initialization is attempted.
  if (FindSpec(name) == nullptr) {
    if (throwOnError) {
      ThrowErrorCode(ACCESS_INVALID_RT_FUNC_NAME_ERR);
    }
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  EnsureInitializedLocked();
  return ResolveLocked(name, throwOnError);
}

#endif // MS_DEBUGGER
