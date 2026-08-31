# fake CANN runtime / hal 动态库：为 stub 的 dlopen/dlsym 提供最小符号集。
# 独立子目录放置，避免与上方 add_llvm 测试目标同目录触发 LLVM 的 source 检查。
# 指定输出目录，覆盖 LLVM 构建全局的 lib/ 输出设置，
# 与 AclrtStubSmokeTests 的 FAKE_ACLRT_SO_PATH / rpath 保持一致
set(FAKE_LIB_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_library(fake_aclrt SHARED FakeAclrtLib.cpp)
set_target_properties(fake_aclrt PROPERTIES
  OUTPUT_NAME runtime
  LIBRARY_OUTPUT_DIRECTORY ${FAKE_LIB_OUTPUT_DIR})

add_library(fake_hal SHARED FakeHalLib.cpp)
set_target_properties(fake_hal PROPERTIES
  OUTPUT_NAME ascend_hal
  LIBRARY_OUTPUT_DIRECTORY ${FAKE_LIB_OUTPUT_DIR})

foreach(fake_lib fake_aclrt fake_hal)
  target_include_directories(${fake_lib} PRIVATE ${LLDB_SOURCE_DIR}/tools/msdebug)
  target_compile_definitions(${fake_lib} PRIVATE MS_DEBUGGER=1)
endforeach()
