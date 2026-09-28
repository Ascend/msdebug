# SecurityFlags.cmake — msDebug 安全编译选项
#
# 源码模式下由 llvm/CMakeLists.txt 的 "compiler security option" 块处理
# （add_compile_options / add_link_options 目录级继承到 LLDB 子目录）。
# 预编译模式（standalone LLDB）顶层是 lldb/CMakeLists.txt，不经 llvm/CMakeLists.txt，
# 故通过 CMAKE_PROJECT_INCLUDE 注入此文件，确保安全选项在两种构建模式下均生效。
if (MS_DEBUGGER)
  add_compile_options("-Wall")
  add_compile_options("-fPIC")
  add_compile_options("-fstack-protector-all")
  add_link_options("-Wl,-z,relro")
  add_link_options("-Wl,-z,now")
  add_link_options("-Wl,-z,noexecstack")
  add_link_options("-s")
  # -pie 仅用于可执行文件，不能用于共享库。CMake 4.1+ 的 add_link_options 会将 -pie
  # 传递到共享库链接命令中，导致 "undefined reference to main" 错误，因此改用
  # CMAKE_EXE_LINKER_FLAGS 确保 -pie 只作用于可执行目标。
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
  # -fPIC，保证探测结果和你真实工程编译环境一致，避免 "探测通过、实际编译失败"。
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fPIC")
  # 预编译包在低 glibc（2.17）上构建、分发到更高 glibc：fortify 的 memset 内联会引用
  # __warn_memset_zero_len（glibc>=2.27 已移除），导致使用端 undefined reference。
  # 预编译构建（Dockerfile 设 MSDEBUG_PREBUILT=1）关闭 fortify，常规构建保持开启。
  if (DEFINED ENV{MSDEBUG_PREBUILT})
    add_compile_options("-U_FORTIFY_SOURCE")
  else()
    add_compile_options("-D_FORTIFY_SOURCE=2")
  endif()
  add_compile_options("-ftrapv")
  set(CMAKE_SKIP_RPATH TRUE)
endif()
