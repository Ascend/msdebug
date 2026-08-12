include(ExternalProject)
include(ProcessorCount)

find_program(NINJA NAMES ninja ninja-build)
find_program(MAKE NAMES make gmake)

if(NINJA)
    message(STATUS "Using Ninja as build system")
    set(GENERATOR "Ninja")
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 1)
    endif()
    set(BUILD_COMMAND ${NINJA} -j ${NPROC})
    set(INSTALL_COMMAND ${NINJA} install)
else()
    message(STATUS "Falling back to Make as build system")
    set(GENERATOR "Unix Makefiles")
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 1)
    endif()
    set(BUILD_COMMAND ${MAKE} -j ${NPROC})
    set(INSTALL_COMMAND ${MAKE} install)
endif()

set(MSDEBUG_MI_SOURCE_DIR "${ROOT_DIR}/msdebug-mi")
set(MSDEBUG_MI_BINARY_DIR "${PROJECT_BUILD_DIR}/msdebug-mi-build")

# 将顶层检测到的 ccache 透传给 msdebug-mi 子项目，加速重复构建
set(MSDEBUG_MI_CCACHE_ARGS "")
if(CCACHE_PROGRAM)
    list(APPEND MSDEBUG_MI_CCACHE_ARGS
        -DCMAKE_C_COMPILER_LAUNCHER=${CCACHE_PROGRAM}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CCACHE_PROGRAM}
    )
endif()

# msdebug-mi 始终使用 find_package(LLVM CONFIG) 原流程；
# 预编译模式下 LLVM_DIR 指向 prebuilt 包接口，LLVM_BUILD_BINARY_DIR 指向 standalone LLDB 构建目录
if(USE_PREBUILT_LLVM)
    set(MSDEBUG_MI_EXTRA_CMAKE_ARGS
        -DLLVM_DIR=${LLVM_PREBUILT_LIB_DIR}/cmake/llvm
        -DLLVM_BUILD_BINARY_DIR=${LLDB_STANDALONE_BUILD_DIR}
        -DMSDEBUG_SOURCE_DIR=${ROOT_DIR}
    )
    set(MSDEBUG_MI_DEPENDS llvm_project)
else()
    set(MSDEBUG_MI_EXTRA_CMAKE_ARGS
        -DLLVM_DIR=${LLVM_BINARY_DIR}/lib/cmake/llvm
        -DLLVM_BUILD_BINARY_DIR=${LLVM_BINARY_DIR}
    )
    set(MSDEBUG_MI_DEPENDS llvm_project)
endif()

ExternalProject_Add(msdebug_mi_project
    SOURCE_DIR ${MSDEBUG_MI_SOURCE_DIR}
    BINARY_DIR ${MSDEBUG_MI_BINARY_DIR}
    CMAKE_ARGS
        -G ${GENERATOR}
        -DMS_DEBUGGER=1
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DLibEdit_LIBRARIES=${LIBEDIT_INSTALL_DIR}/lib/libedit.so.0
        -DLibEdit_INCLUDE_DIRS=${LIBEDIT_INSTALL_DIR}/include
        -DLLVM_ENABLE_LANGUAGE_C=OFF
        -DMS_DEBUGGER_LIBEDIT=${LIBEDIT_INSTALL_DIR}/lib
        -DMS_DEBUGGER_NCURSES=${NCURSES_INSTALL_DIR}/lib
        -DCMAKE_PREFIX_PATH=${LIBEDIT_INSTALL_DIR}/lib
        -DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath-link,${LIBEDIT_INSTALL_DIR}/lib
        ${MSDEBUG_MI_CCACHE_ARGS}
        ${MSDEBUG_MI_EXTRA_CMAKE_ARGS}
    USES_TERMINAL_BUILD TRUE
    BUILD_COMMAND ${BUILD_COMMAND}
    INSTALL_COMMAND ${INSTALL_COMMAND}
    BUILD_ALWAYS ON
    DEPENDS ${MSDEBUG_MI_DEPENDS}
)
