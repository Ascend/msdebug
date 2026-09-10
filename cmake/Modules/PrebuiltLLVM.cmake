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
