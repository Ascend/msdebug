# 黑盒冒烟测试：直接编译真实 stub 源码并调用三个新劫持函数，
# 配合 fakelibs/ 下编译的 fake CANN runtime/hal 库走通完整 launch 链路。
# 单独的 add_subdirectory 目录，满足 LLVM 构建系统"每个目录仅一个 target"的约束。

# 先加载 fake 库子目录，确保 target 已定义，供下方 add_dependencies 引用
add_subdirectory(fakelibs)

add_lldb_unittest(AclrtStubSmokeTests
  AclrtStubSmokeTest.cpp
  ${LLDB_SOURCE_DIR}/tools/msdebug/aclrt_stub.cpp
  ${LLDB_SOURCE_DIR}/tools/msdebug/runtime_stub.cpp
  ${LLDB_SOURCE_DIR}/tools/msdebug/get_tiling_key.cpp
  ${LLDB_SOURCE_DIR}/tools/msdebug/AscendCommunicationClient.cpp
  ${LLDB_SOURCE_DIR}/tools/msdebug/HijackedLayerManager.cpp
  LINK_LIBS lldbHost lldbUtility dl
  # 与 ProcessLinuxTests 的 -Wl,--wrap=open/ioctl 同款链接期拦截：
  # 拦截 access() 使 EnvCheck 的 /dev/drv_debug 检查通过，单测无需依赖驱动
  -Wl,--wrap=access
  LINK_COMPONENTS Object
  )

# fake 库仅在运行时被 dlopen，不作为链接依赖，
# 必须显式声明构建依赖，否则 check-lldb-unit 不会构建它们，测试将 dlopen 失败
add_dependencies(AclrtStubSmokeTests fake_aclrt fake_hal)

target_include_directories(AclrtStubSmokeTests PRIVATE
  ${LLDB_SOURCE_DIR}/tools/msdebug)
# LLVM 单测构建默认 -fno-exceptions，而 stub 源码（rt_stub_log.h 的 throw/catch）依赖异常，
# 需在目标级重新开启（会排在全局 -fno-exceptions 之后）
target_compile_options(AclrtStubSmokeTests PRIVATE -fexceptions)
target_compile_definitions(AclrtStubSmokeTests PRIVATE MS_DEBUGGER=1
  FAKE_ACLRT_SO_PATH="${CMAKE_CURRENT_BINARY_DIR}/fakelibs/libruntime.so")
# dlopen("libascend_hal.so") 需能命中 fake hal 库
target_link_options(AclrtStubSmokeTests PRIVATE -Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}/fakelibs)
