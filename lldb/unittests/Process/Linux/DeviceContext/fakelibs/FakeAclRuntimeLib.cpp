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

// Minimal fake ACL runtime used by AclRuntimeTests. Only the public ACL
// entry points are exported; the test can drive every return code and inspect
// the arguments that reached aclrtMemcpy / the IPC helpers.

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

int g_init_ret = 0;
int g_get_device_ret = 0;
int g_set_device_ret = 0;
int g_memcpy_ret = 0;
int g_ipc_import_ret = 0;
int g_ipc_close_ret = 0;

int g_current_device = 0;
int g_set_device_count = 0;
int g_ipc_import_count = 0;
int g_ipc_close_count = 0;

std::size_t g_last_dest_max = 0;
std::size_t g_last_count = 0;
int g_last_kind = -1;

} // namespace

extern "C" {

int aclInit(const char *) { return g_init_ret; }

int aclrtGetDevice(int *device) {
  if (device != nullptr) {
    *device = g_current_device;
  }
  return g_get_device_ret;
}

int aclrtSetDevice(int device) {
  ++g_set_device_count;
  g_current_device = device;
  return g_set_device_ret;
}

#ifndef FAKE_SKIP_ACLRT_MEMCPY
int aclrtMemcpy(void *dst, std::size_t dest_max, const void *src,
                std::size_t count, int kind) {
  g_last_dest_max = dest_max;
  g_last_count = count;
  g_last_kind = kind;
  if (g_memcpy_ret == 0 && dst != nullptr && src != nullptr) {
    std::memcpy(dst, src, dest_max < count ? dest_max : count);
  }
  return g_memcpy_ret;
}
#endif

int aclrtIpcMemImportByKey(void **dev_ptr, const char *key, uint64_t flags) {
  ++g_ipc_import_count;
  (void)key;
  (void)flags;
  if (g_ipc_import_ret == 0 && dev_ptr != nullptr) {
    *dev_ptr = reinterpret_cast<void *>(0x12345000ULL);
  }
  return g_ipc_import_ret;
}

int aclrtIpcMemClose(const char *key) {
  ++g_ipc_close_count;
  (void)key;
  return g_ipc_close_ret;
}

// ---- test control surface -------------------------------------------------

void fakeReset() {
  g_init_ret = 0;
  g_get_device_ret = 0;
  g_set_device_ret = 0;
  g_memcpy_ret = 0;
  g_ipc_import_ret = 0;
  g_ipc_close_ret = 0;
  g_current_device = 0;
  g_set_device_count = 0;
  g_ipc_import_count = 0;
  g_ipc_close_count = 0;
  g_last_dest_max = 0;
  g_last_count = 0;
  g_last_kind = -1;
}

void fakeSetInitRet(int value) { g_init_ret = value; }
void fakeSetGetDeviceRet(int value) { g_get_device_ret = value; }
void fakeSetSetDeviceRet(int value) { g_set_device_ret = value; }
void fakeSetMemcpyRet(int value) { g_memcpy_ret = value; }
void fakeSetIpcImportRet(int value) { g_ipc_import_ret = value; }
void fakeSetIpcCloseRet(int value) { g_ipc_close_ret = value; }
void fakeSetCurrentDevice(int value) { g_current_device = value; }

int fakeGetCurrentDevice() { return g_current_device; }
int fakeGetSetDeviceCount() { return g_set_device_count; }
int fakeGetIpcImportCount() { return g_ipc_import_count; }
int fakeGetIpcCloseCount() { return g_ipc_close_count; }
std::size_t fakeGetLastDestMax() { return g_last_dest_max; }
std::size_t fakeGetLastCount() { return g_last_count; }
int fakeGetLastKind() { return g_last_kind; }

} // extern "C"
