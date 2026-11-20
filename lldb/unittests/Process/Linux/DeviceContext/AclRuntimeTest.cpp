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

#include <dlfcn.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#ifndef FAKE_ACL_RT_LIB_PATH
#define FAKE_ACL_RT_LIB_PATH "libacl_rt.so"
#endif
#ifndef FAKE_ACL_RT_MISSING_LIB_PATH
#define FAKE_ACL_RT_MISSING_LIB_PATH "libacl_rt_missing.so"
#endif

using namespace lldb_private;

class AclRuntimeTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_fake_lib = dlopen(FAKE_ACL_RT_LIB_PATH, RTLD_NOW | RTLD_GLOBAL);
    ASSERT_NE(m_fake_lib, nullptr) << dlerror();
    auto reset = Sym<void (*)()>("fakeReset");
    ASSERT_NE(reset, nullptr);
    reset();
  }

  void TearDown() override {
    if (m_fake_lib != nullptr) {
      dlclose(m_fake_lib);
      m_fake_lib = nullptr;
    }
  }

  template <typename Fn> Fn Sym(const char *name) const {
    return reinterpret_cast<Fn>(dlsym(m_fake_lib, name));
  }

  void *m_fake_lib = nullptr;
};

TEST_F(AclRuntimeTest, InitializeResolvesPublicSymbols) {
  AclRuntime runtime(FAKE_ACL_RT_LIB_PATH);
  EXPECT_TRUE(runtime.Initialize().Success());
}

TEST_F(AclRuntimeTest, InitializeFailsWhenRequiredSymbolMissing) {
  AclRuntime runtime(FAKE_ACL_RT_MISSING_LIB_PATH);
  Status error = runtime.Initialize();
  ASSERT_TRUE(error.Fail());
  const char *message = error.AsCString();
  ASSERT_NE(message, nullptr);
  EXPECT_NE(std::string(message).find("aclrtMemcpy"), std::string::npos);
}

TEST_F(AclRuntimeTest, InitializeFailsWhenLibraryIsMissing) {
  AclRuntime runtime("/nonexistent/libacl_rt.so");
  EXPECT_TRUE(runtime.Initialize().Fail());
}

TEST_F(AclRuntimeTest, MemcpyForwardsDestMaxAndCount) {
  AclRuntime runtime(FAKE_ACL_RT_LIB_PATH);
  ASSERT_TRUE(runtime.Initialize().Success());

  char src[16] = {0};
  char dst[16] = {0};
  EXPECT_TRUE(
      runtime.Memcpy(dst, 8, src, 12, AclRuntime::Direction::HostToDevice)
          .Success());

  auto get_dest_max = Sym<std::size_t (*)()>("fakeGetLastDestMax");
  auto get_count = Sym<std::size_t (*)()>("fakeGetLastCount");
  auto get_kind = Sym<int (*)()>("fakeGetLastKind");
  ASSERT_NE(get_dest_max, nullptr);
  ASSERT_NE(get_count, nullptr);
  ASSERT_NE(get_kind, nullptr);
  EXPECT_EQ(get_dest_max(), 8u);
  EXPECT_EQ(get_count(), 12u);
  EXPECT_EQ(get_kind(), static_cast<int>(AclRuntime::Direction::HostToDevice));
}

TEST_F(AclRuntimeTest, IpcImportAndClose) {
  AclRuntime runtime(FAKE_ACL_RT_LIB_PATH);
  ASSERT_TRUE(runtime.Initialize().Success());

  void *shared_addr = nullptr;
  std::vector<char> key(65, 'k');
  EXPECT_TRUE(runtime.IpcMemImportByKey(&shared_addr, key).Success());
  EXPECT_NE(shared_addr, nullptr);
  EXPECT_TRUE(runtime.IpcMemClose(key).Success());

  auto import_count = Sym<int (*)()>("fakeGetIpcImportCount");
  auto close_count = Sym<int (*)()>("fakeGetIpcCloseCount");
  ASSERT_NE(import_count, nullptr);
  ASSERT_NE(close_count, nullptr);
  EXPECT_EQ(import_count(), 1);
  EXPECT_EQ(close_count(), 1);
}

TEST_F(AclRuntimeTest, EnsureDeviceOnlySetsOnMismatch) {
  AclRuntime runtime(FAKE_ACL_RT_LIB_PATH);
  ASSERT_TRUE(runtime.Initialize().Success());

  auto set_current = Sym<void (*)(int)>("fakeSetCurrentDevice");
  auto get_current = Sym<int (*)()>("fakeGetCurrentDevice");
  auto get_set_count = Sym<int (*)()>("fakeGetSetDeviceCount");
  ASSERT_NE(set_current, nullptr);
  ASSERT_NE(get_current, nullptr);
  ASSERT_NE(get_set_count, nullptr);

  set_current(2);
  EXPECT_TRUE(runtime.EnsureDevice(2).Success());
  EXPECT_EQ(get_set_count(), 0);
  EXPECT_EQ(get_current(), 2);

  EXPECT_TRUE(runtime.EnsureDevice(5).Success());
  EXPECT_EQ(get_set_count(), 1);
  EXPECT_EQ(get_current(), 5);
}

#endif // MS_DEBUGGER
