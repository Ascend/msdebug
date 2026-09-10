# LLVMTestToolStubs.cmake — UT（预编译模式）专用
#
# 背景：LLDB 以 standalone 构建开启 LLDB_INCLUDE_TESTS=ON 时，
# lldb/test/CMakeLists.txt 会无条件 add_dependencies 依赖 LLVM 二进制工具
# （llvm-nm / llvm-readobj / llvm-ar 等）。这些 tool 只存在于源码模式的全量
# 构建图里，预编译模式链接 prebuilt .a 并没有它们，导致 configure 失败。
# gtest 单测实际并不调用这些工具，因此注入同名空 target 满足依赖即可。
#
# 通过 -DCMAKE_PROJECT_INCLUDE 注入到 standalone LLDB 顶层 project() 之后。
foreach(_llvm_ut_tool
    llc lli llvm-config llvm-dwarfdump llvm-dwp llvm-nm llvm-mc
    llvm-objcopy llvm-pdbutil llvm-readobj llvm-ar)
    if(NOT TARGET ${_llvm_ut_tool})
        add_custom_target(${_llvm_ut_tool})
    endif()
endforeach()
unset(_llvm_ut_tool)
