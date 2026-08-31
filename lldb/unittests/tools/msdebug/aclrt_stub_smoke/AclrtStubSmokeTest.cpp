/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */

// 黑盒冒烟测试：直接调用三个新劫持函数（aclrtLaunchSIMTKernelWithArgsArrayImpl 等），
// 验证在 fake runtime/hal 库支撑下能完整走完 劫持→resolve→pre→origin→post 链路，
// 正常返回 ACL_SUCCESS，不崩溃、不卡死。

#include "acl.h"

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

// 链接期拦截 access()（见 CMakeLists 的 -Wl,--wrap=access），
// 与 AscendProcessLinuxTest 拦截 open/ioctl 同款模式：
// 伪造 /dev/drv_debug 存在，使 EnvCheck 的设备检查通过，单测无需 NPU 驱动。
extern "C" int __real_access(const char *pathname, int mode);
extern "C" int __wrap_access(const char *pathname, int mode) {
  if (pathname != nullptr && strcmp(pathname, "/dev/drv_debug") == 0) {
    return 0;
  }
  return __real_access(pathname, mode);
}

namespace {

#ifndef FAKE_ACLRT_SO_PATH
#define FAKE_ACLRT_SO_PATH "libruntime.so"
#endif

std::string g_toolkitPath;

class AclrtStubSmokeTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    // 构造 $ASCEND_TOOLKIT_HOME/lib64/libruntime.so（symlink 到 fake 库）
    std::string tmpl = "/tmp/msdebug_stub_ut_XXXXXX";
    std::vector<char> dirTmpl(tmpl.begin(), tmpl.end());
    dirTmpl.push_back('\0');
    char *dir = mkdtemp(dirTmpl.data());
    if (dir == nullptr) {
      FAIL() << "mkdtemp failed";
      return;
    }
    g_toolkitPath = dir;
    std::string lib64 = g_toolkitPath + "/lib64";
    if (mkdir(lib64.c_str(), 0755) != 0) {
      FAIL() << "mkdir " << lib64 << " failed";
      return;
    }
    if (symlink(FAKE_ACLRT_SO_PATH, (lib64 + "/libruntime.so").c_str()) != 0) {
      FAIL() << "symlink fake libruntime.so failed";
      return;
    }
    if (setenv("ASCEND_TOOLKIT_HOME", g_toolkitPath.c_str(), 1) != 0 ||
        setenv("DEBUGGER_RT_STUB_LOG", "0", 1) != 0) {
      FAIL() << "setenv failed";
      return;
    }

    // 触发 StubInit：access() 已被 __wrap_access 拦截，/dev/drv_debug 检查必然通过，
    // 若仍抛错说明 fake 库/环境搭建有问题，直接失败暴露。
    ASSERT_NO_THROW(
        aclrtLaunchKernelWithArgsArrayImpl(nullptr, 1, nullptr, nullptr, nullptr));
  }

  static void TearDownTestSuite() {
    unsetenv("ASCEND_TOOLKIT_HOME");
    if (!g_toolkitPath.empty()) {
      unlink((g_toolkitPath + "/lib64/libruntime.so").c_str());
      rmdir((g_toolkitPath + "/lib64").c_str());
      rmdir(g_toolkitPath.c_str());
    }
  }

  // 切换 fake aclrtGetFuncBySymbolImpl 行为：0=解析成功(核函数符号)，非0=解析失败(funcHandle)
  void SetResolveMode(int mode) {
    void *handle = dlopen(FAKE_ACLRT_SO_PATH, RTLD_NOW);
    ASSERT_NE(handle, nullptr) << dlerror();
    auto setter = reinterpret_cast<void (*)(int)>(dlsym(handle, "fakeSetResolveMode"));
    ASSERT_NE(setter, nullptr) << dlerror();
    setter(mode);
    dlclose(handle);
  }
};

// func 作为核函数符号：fake aclrtGetFuncBySymbolImpl 解析成功，返回假 handle
TEST_F(AclrtStubSmokeTest, SimtWithArgsArrayRunsWithKernelSymbol) {
  SetResolveMode(0);
  void *func = reinterpret_cast<void *>(0x1000);
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithArgsArrayImpl(func, grid, block, 0,
                                                   nullptr, nullptr, nullptr));
}

// func 作为 funcHandle：fake aclrtGetFuncBySymbolImpl 解析失败，直接使用入参
TEST_F(AclrtStubSmokeTest, SimtWithArgsArrayRunsWithFuncHandle) {
  SetResolveMode(1);
  void *func = reinterpret_cast<void *>(0x1000);
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithArgsArrayImpl(func, grid, block, 0,
                                                   nullptr, nullptr, nullptr));
}

// func 为空：不应崩溃
TEST_F(AclrtStubSmokeTest, SimtWithArgsArrayRunsWithNullFunc) {
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithArgsArrayImpl(nullptr, grid, block, 0,
                                                   nullptr, nullptr, nullptr));
}

TEST_F(AclrtStubSmokeTest, KernelWithArgsArrayRunsWithKernelSymbol) {
  SetResolveMode(0);
  void *func = reinterpret_cast<void *>(0x1000);
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchKernelWithArgsArrayImpl(func, 1, nullptr, nullptr, nullptr));
}

TEST_F(AclrtStubSmokeTest, KernelWithArgsArrayRunsWithFuncHandle) {
  SetResolveMode(1);
  void *func = reinterpret_cast<void *>(0x1000);
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchKernelWithArgsArrayImpl(func, 1, nullptr, nullptr, nullptr));
}

TEST_F(AclrtStubSmokeTest, KernelWithArgsArrayRunsWithNullFunc) {
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchKernelWithArgsArrayImpl(nullptr, 1, nullptr, nullptr, nullptr));
}

TEST_F(AclrtStubSmokeTest, SimtWithHostArgsRunsWithKernelSymbol) {
  SetResolveMode(0);
  void *func = reinterpret_cast<void *>(0x1000);
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithHostArgsImpl(func, grid, block, 0, nullptr,
                                                  nullptr, nullptr, 0, nullptr, 0));
}

TEST_F(AclrtStubSmokeTest, SimtWithHostArgsRunsWithFuncHandle) {
  SetResolveMode(1);
  void *func = reinterpret_cast<void *>(0x1000);
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithHostArgsImpl(func, grid, block, 0, nullptr,
                                                  nullptr, nullptr, 0, nullptr, 0));
}

TEST_F(AclrtStubSmokeTest, SimtWithHostArgsRunsWithNullFunc) {
  dim3 grid{1, 1, 1};
  dim3 block{1, 1, 1};
  EXPECT_EQ(ACL_SUCCESS,
            aclrtLaunchSIMTKernelWithHostArgsImpl(nullptr, grid, block, 0, nullptr,
                                                  nullptr, nullptr, 0, nullptr, 0));
}

} // namespace
