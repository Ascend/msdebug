# PrebuiltLLVM.cmake — 预编译LLVM/LLDB库配置
# 设置 USE_PREBUILT_LLVM=ON 时，跳过LLVM源码编译，直接使用预编译的静态库和头文件
# 导出的变量会被 Msdebug_mi.cmake 传递给 msdebug-mi 子构建

if(NOT USE_PREBUILT_LLVM)
    message(STATUS "Prebuilt LLVM: disabled, using source build")
    return()
endif()

# ---------- 架构检测（固定当前机器架构，不支持交叉打包）----------
if(NOT LLVM_PREBUILT_ARCH)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(LLVM_PREBUILT_ARCH "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(LLVM_PREBUILT_ARCH "aarch64")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

# ---------- 预编译库根目录 ----------
if(NOT LLVM_PREBUILT_ROOT)
    set(LLVM_PREBUILT_ROOT "${CMAKE_SOURCE_DIR}/prebuilt")
endif()

# ---------- 预编译库路径 ----------
set(LLVM_PREBUILT_LIB_DIR "${LLVM_PREBUILT_ROOT}/${LLVM_PREBUILT_ARCH}/lib")
set(LLVM_PREBUILT_INCLUDE_DIR "${LLVM_PREBUILT_ROOT}/${LLVM_PREBUILT_ARCH}/include")

# ---------- 验证 ----------
if(NOT IS_DIRECTORY "${LLVM_PREBUILT_LIB_DIR}")
    message(FATAL_ERROR "Prebuilt library directory not found: ${LLVM_PREBUILT_LIB_DIR}")
endif()
# 头文件不预先提供：LLVM 接口（含头文件）在预编译模式现由 LLVM 源码 configure 生成
if(NOT EXISTS "${LLVM_PREBUILT_LIB_DIR}/libLLVMSupport.a")
    message(FATAL_ERROR "Required prebuilt library not found: ${LLVM_PREBUILT_LIB_DIR}/libLLVMSupport.a")
endif()
if(NOT EXISTS "${LLVM_PREBUILT_LIB_DIR}/libclangAST.a")
    message(FATAL_ERROR "Required prebuilt library not found: ${LLVM_PREBUILT_LIB_DIR}/libclangAST.a")
endif()

# ---------- 构建元数据校验（glibc）----------
# prebuilt 包必须携带 prebuilt_meta.json；glibc 仅向后兼容，若由更高 glibc 构建，
# 本机会因缺少新符号而失败。不按 GCC 版本判断（低/高 GCC 未必决定符号兼容）。
set(_prebuilt_meta "${LLVM_PREBUILT_ROOT}/${LLVM_PREBUILT_ARCH}/prebuilt_meta.json")
if(NOT EXISTS "${_prebuilt_meta}")
    message(FATAL_ERROR
        "Prebuilt LLVM package is incomplete: ${_prebuilt_meta} not found.\n"
        "Fix: re-download the prebuilt package, or configure with "
        "-DUSE_PREBUILT_LLVM=OFF (python build.py --no-prebuilt).")
endif()

file(READ "${_prebuilt_meta}" _meta_json)
string(JSON _need_glibc ERROR_VARIABLE _json_err GET "${_meta_json}" min_glibc)
if(NOT _need_glibc MATCHES "^[0-9]+\\.[0-9]+")
    string(JSON _need_glibc ERROR_VARIABLE _json_err GET "${_meta_json}" glibc_version)
endif()

execute_process(COMMAND getconf GNU_LIBC_VERSION
                OUTPUT_VARIABLE _libc_out OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
string(REGEX MATCH "[0-9]+\\.[0-9]+" _host_glibc "${_libc_out}")

if(_need_glibc MATCHES "^[0-9]+\\.[0-9]+" AND _host_glibc MATCHES "^[0-9]+\\.[0-9]+"
   AND _host_glibc VERSION_LESS _need_glibc)
    message(FATAL_ERROR
        "Prebuilt LLVM was built with glibc ${_need_glibc}, but this machine has glibc ${_host_glibc}.\n"
        "Fix: rebuild the prebuilt package against an older glibc, or configure with "
        "-DUSE_PREBUILT_LLVM=OFF (python build.py --no-prebuilt).")
endif()

message(STATUS "  Built with:     glibc ${_need_glibc} (this machine: glibc ${_host_glibc})")

message(STATUS "=== Prebuilt LLVM Configuration ===")
message(STATUS "  Architecture:   ${LLVM_PREBUILT_ARCH}")
message(STATUS "  Library dir:    ${LLVM_PREBUILT_LIB_DIR}")
message(STATUS "  Include dir:    ${LLVM_PREBUILT_INCLUDE_DIR}")
message(STATUS "  Version:        ${LLVM_PREBUILT_VERSION}")
message(STATUS "===================================")

# 导出给 LLVM.cmake / Msdebug_mi.cmake 使用
set(LLVM_PREBUILT_LIB_DIR "${LLVM_PREBUILT_LIB_DIR}" CACHE INTERNAL "Prebuilt LLVM library directory")
set(LLVM_PREBUILT_INCLUDE_DIR "${LLVM_PREBUILT_INCLUDE_DIR}" CACHE INTERNAL "Prebuilt LLVM include directory")
set(LLVM_PREBUILT_ARCH "${LLVM_PREBUILT_ARCH}" CACHE INTERNAL "Prebuilt LLVM architecture")
