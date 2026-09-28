# MsdebugProjectInclude.cmake — 预编译模式 standalone LLDB 的 project include
#
# 通过 -DCMAKE_PROJECT_INCLUDE 注入到 lldb/CMakeLists.txt 的 project() 之后。
# 此时 add_compile_options / add_link_options 作用于顶层目录，会继承到所有 LLDB target
# （liblldb / lldb-server / runtime_stub / lldb driver），与源码模式下
# llvm/CMakeLists.txt 的目录级继承效果一致。
#
# 源码模式下安全选项由 llvm/CMakeLists.txt 处理，此文件仅预编译模式使用。
include(${CMAKE_CURRENT_LIST_DIR}/SecurityFlags.cmake)

# UT 模式：lldb/test 基建无条件依赖 LLVM 二进制工具（llvm-nm/llvm-ar 等），
# 预编译模式无这些 tool target，注入 stub 满足 configure 依赖。
if(LLDB_INCLUDE_TESTS)
  include(${CMAKE_CURRENT_LIST_DIR}/LLVMTestToolStubs.cmake)
endif()
