cmake_minimum_required(VERSION 3.20.2)

set(MAX_SUPPORTED_CMAKE_VERSION "3.31.10")
if(CMAKE_VERSION VERSION_GREATER MAX_SUPPORTED_CMAKE_VERSION)
    message(FATAL_ERROR 
        "CMake ${CMAKE_VERSION} is too new! The maximum supported version is ${MAX_SUPPORTED_CMAKE_VERSION}."
    )
else()
    message(STATUS "CMake version check passed: ${CMAKE_VERSION} ≤ ${MAX_SUPPORTED_CMAKE_VERSION}")
endif()

project(MindStudio-Debugger)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
set(PROJECT_BUILD_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Root build directory")

# 检查是否可用 ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found, using native compiler.")
endif()

# 为了适用于工程本身构建，也适用于其它工程调用此工程，CMAKE_INSTALL_PREFIX的判断方式需要判断是否未设定或者系统默认路径
if (${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local" OR ${CMAKE_INSTALL_PREFIX} STREQUAL "/usr" OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${ROOT_DIR}/output CACHE STRING "install prefix" FORCE)
  message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}")
endif() 


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

if(DEFINED ENABLE_LLDB_TESTS)
    set(LLDB_INCLUDE_TESTS ${ENABLE_LLDB_TESTS} CACHE BOOL "Enable LLDB tests")
else()
    set(LLDB_INCLUDE_TESTS OFF CACHE BOOL "Enable LLDB tests")
endif()


# Include modules
include(cmake/Modules/Ncurses.cmake)
include(cmake/Modules/Libedit.cmake)
include(cmake/Modules/LLVM.cmake)
include(cmake/Modules/Msdebug_mi.cmake)
include(cmake/Modules/PreparePackage.cmake)

set(ALL_EXTERNAL_TARGETS
    ncurses_project
    libedit_project
    llvm_project
    msdebug_mi_project
)


add_custom_target(build_debugger 
        COMMENT "Building all Debugger components"
        DEPENDS ${ALL_EXTERNAL_TARGETS}
)

include(cmake/Pack_debugger.cmake)

add_dependencies(package_debugger build_debugger)