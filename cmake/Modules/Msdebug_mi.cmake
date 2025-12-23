include(ExternalProject)
include(ProcessorCount)

find_program(NINJA NAMES ninja ninja-build)
find_program(MAKE NAMES make gmake)

if(NINJA)
    message(STATUS "Using Ninja as build system")
    set(GENERATOR "Ninja")  # Preferred generator
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 1)
    endif()
    set(BUILD_COMMAND ${NINJA} -j ${NPROC})
    set(INSTALL_COMMAND ${NINJA} install)
else()
    message(STATUS "Falling back to Make as build system")
    set(GENERATOR "Unix Makefiles")  # Fallback generator
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 1)
    endif()
    set(BUILD_COMMAND ${MAKE} -j ${NPROC})
    set(INSTALL_COMMAND ${MAKE} install)
endif()

set(MSDEBUG_MI_SOURCE_DIR "${ROOT_DIR}/msdebug-mi")
set(MSDEBUG_MI_BINARY_DIR "${PROJECT_BUILD_DIR}/msdebug-mi-build")

ExternalProject_Add(msdebug_mi_project
    SOURCE_DIR ${MSDEBUG_MI_SOURCE_DIR}
    BINARY_DIR ${MSDEBUG_MI_BINARY_DIR}
    CMAKE_ARGS
        -G ${GENERATOR}  # Uses appropriate generator automatically
        -DMS_DEBUGGER=1
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DLLVM_DIR=${LLVM_BINARY_DIR}/lib/cmake/llvm
        -DLLVM_BUILD_BINARY_DIR=${LLVM_BINARY_DIR}
        -DLibEdit_LIBRARIES=${LIBEDIT_INSTALL_DIR}/lib/libedit.so.0
        -DLibEdit_INCLUDE_DIRS=${LIBEDIT_INSTALL_DIR}/include
        -DLLVM_BUILD_BINARY_DIR=${LLVM_BINARY_DIR}/lib
        -DLLVM_ENABLE_LANGUAGE_C=OFF
        -DMS_DEBUGGER_LIBEDIT=${LIBEDIT_INSTALL_DIR}/lib
        -DMS_DEBUGGER_NCURSES=${NCURSES_INSTALL_DIR}/lib
        -DCMAKE_PREFIX_PATH=${LIBEDIT_INSTALL_DIR}/lib
        -DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath-link,${LIBEDIT_INSTALL_DIR}/lib

    USES_TERMINAL_BUILD TRUE    
    BUILD_COMMAND ${BUILD_COMMAND}
    INSTALL_COMMAND ${INSTALL_COMMAND}
    BUILD_ALWAYS OFF
    DEPENDS llvm_project
)