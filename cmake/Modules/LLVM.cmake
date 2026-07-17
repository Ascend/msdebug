include(ExternalProject)
include(ProcessorCount)
ProcessorCount(NPROC)

set(LLVM_SOURCE_DIR "${ROOT_DIR}/llvm")
set(LLVM_BINARY_DIR "${PROJECT_BUILD_DIR}/llvm-build" CACHE PATH "LLVM build")

find_program(NINJA ninja)
find_program(MAKE make)

if(NINJA)
    set(LLVM_GENERATOR "Ninja")
    set(LLVM_BUILD_CMD ${NINJA} -j ${NPROC} lldb lldb-server runtime_stub)
    if(LLDB_INCLUDE_TESTS)
        list(APPEND LLVM_BUILD_CMD check-lldb-unit)
    endif()

    set(LLVM_INSTALL_CMD ${NINJA} install-msdebug)
elseif(MAKE)
    set(LLVM_GENERATOR "Unix Makefiles")
    set(LLVM_BUILD_CMD ${MAKE} -j ${NPROC} lldb lldb-server runtime_stub)
    if(LLDB_INCLUDE_TESTS)
        list(APPEND LLVM_BUILD_CMD check-lldb-unit)
    endif()

    set(LLVM_INSTALL_CMD ${MAKE} install-msdebug)
else()
    message(FATAL_ERROR "Neither ninja nor make found!")
endif()

list(APPEND CMAKE_PREFIX_PATH "${LIBEDIT_INSTALL_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${NCURSES_INSTALL_DIR}")

# 将顶层检测到的 ccache 透传给 LLVM 子项目（占整体编译时长 95%+），显著加速重复构建
set(LLVM_CCACHE_ARGS "")
if(CCACHE_PROGRAM)
    list(APPEND LLVM_CCACHE_ARGS
        -DCMAKE_C_COMPILER_LAUNCHER=${CCACHE_PROGRAM}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CCACHE_PROGRAM}
    )
endif()

# LLVM 工具(opt/llc/bugpoint 等)非产品构建所必需，仅在跑 lit/单元测试时才需要(FileCheck 等)，关闭以省编译
if(LLDB_INCLUDE_TESTS)
    set(LLVM_BUILD_TOOLS_FLAG ON)
else()
    set(LLVM_BUILD_TOOLS_FLAG OFF)
endif()

ExternalProject_Add(llvm_project
    SOURCE_DIR ${LLVM_SOURCE_DIR}
    BINARY_DIR ${LLVM_BINARY_DIR}
    CMAKE_ARGS
        -G "${LLVM_GENERATOR}"
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DLLVM_ENABLE_PROJECTS=clang
        -DLLVM_ENABLE_ZSTD=OFF
        -DLLDB_ENABLE_LIBXML2=OFF
        -DLLDB_ENABLE_PYTHON=OFF
        -DLLDB_ENABLE_LIBEDIT=ON
        -DLLVM_BUILD_TOOLS=${LLVM_BUILD_TOOLS_FLAG}
        -DLLDB_INCLUDE_TESTS=${LLDB_INCLUDE_TESTS}
        -DROOT_DIR=${ROOT_DIR}
        -DNCURSES_LIB_DIR=${NCURSES_LIB_DIR}
        -DLIBEDIT_LIBRARY=${LIBEDIT_LIBRARY}
        -DMS_DEBUGGER=1
        -DLibEdit_LIBRARIES=${LIBEDIT_INSTALL_DIR}/lib/libedit.so.0
        -DLibEdit_INCLUDE_DIRS=${LIBEDIT_INSTALL_DIR}/include
        -DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath-link,${LIBEDIT_INSTALL_DIR}/lib
        -DCURSES_NCURSES_LIBRARY=${NCURSES_INSTALL_DIR}/lib/libncurses.so
        -DTerminfo_LIBRARIES=${NCURSES_INSTALL_DIR}/lib/libtinfo.so
        -DCURSES_LIBRARY=${NCURSES_INSTALL_DIR}/lib/libtinfo.so
        -DCURSES_INCLUDE_PATH=${NCURSES_INSTALL_DIR}/include
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DHISTEDIT_FILE=${LIBEDIT_INSTALL_DIR}/include/histedit.h
        -DMS_DEBUGGER_LIBEDIT=${LIBEDIT_INSTALL_DIR}/lib
        -DMS_DEBUGGER_NCURSES=${NCURSES_INSTALL_DIR}/lib
        -DCMAKE_VERBOSE_MAKEFILE=OFF
        ${LLVM_CCACHE_ARGS}
    CMAKE_CACHE_ARGS
        -DLLVM_TARGETS_TO_BUILD:STRING=AArch64\;X86
        -DLLVM_ENABLE_PROJECT_LIST:STRING=clang\;lldb
        -DLLVM_EXTERNAL_PROJECTS:STRING=lldb
        -DLLVM_EXTERNAL_LLDB_SOURCE_DIR:PATH=${CMAKE_SOURCE_DIR}/lldb
        -DCMAKE_BUILD_TYPE:STRING=Release
    USES_TERMINAL_BUILD TRUE
    BUILD_COMMAND ${LLVM_BUILD_CMD}
    INSTALL_COMMAND ${LLVM_INSTALL_CMD}
    BUILD_ALWAYS TRUE
    DEPENDS libedit_project
)
