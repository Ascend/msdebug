include(ExternalProject)
include(ProcessorCount)

set(NCURSES_SOURCE_DIR "${ROOT_DIR}/third-party/ncurses")

set(NCURSES_BUILD_DIR "${PROJECT_BUILD_DIR}/ncurses/build")
set(NCURSES_INSTALL_DIR "${PROJECT_BUILD_DIR}/ncurses/install" CACHE PATH "Ncurses install directory")
set(NCURSES_OUTPUT_DIR <BINARY_DIR> CACHE STRING "Path to ncurses")

file(MAKE_DIRECTORY "${NCURSES_BUILD_DIR}" "${NCURSES_INSTALL_DIR}")

set(NCURSES_LINKFLAGS "-Wl,-z,now -s")
set(NCURSES_CFLAGS "-fstack-protector-all -ftrapv")
set(NCURSES_CPPFLAGS "-D_FORTIFY_SOURCE=2 -O2")

find_program(MAKE make)
if(NOT MAKE)
    message(FATAL_ERROR "make tool not found!")
endif()

file(MAKE_DIRECTORY ${NCURSES_INSTALL_DIR})

file(GLOB_RECURSE NCURSES_TAR_FILE_GLOB ${NCURSES_SOURCE_DIR}/ncurses-*.tar.gz)
list(GET NCURSES_TAR_FILE_GLOB 0 NCURSES_TAR_FILE)
message(STATUS "Use ncurses tar file: ${NCURSES_TAR_FILE}")

add_custom_command(
    OUTPUT ${NCURSES_SOURCE_DIR}/configure
    COMMENT "Extract ncurses component from ${NCURSES_TAR_FILE}"
    # 跳过最外层目录，直接将内容解压到当前目录
    COMMAND git lfs install
    COMMAND git lfs pull
    COMMAND tar xf ${NCURSES_TAR_FILE} --strip-components=1
    DEPENDS ${NCURSES_TAR_FILE}
    WORKING_DIRECTORY ${NCURSES_SOURCE_DIR}
)

add_custom_target(
    ncurses_extract_target
    ALL
    DEPENDS ${NCURSES_SOURCE_DIR}/configure
    COMMENT "Custom target for extracting ncurses tar file"
)

# 检测 GCC 11，没有则 fallback 到系统默认 gcc
find_program(NCURSES_CC gcc PATHS /opt/gcc11-glibc2.17/bin NO_DEFAULT_PATH)
if(NOT NCURSES_CC)
    find_program(NCURSES_CC gcc)
endif()
message(STATUS "Ncurses CC: ${NCURSES_CC}")

ExternalProject_Add(ncurses_project
    DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory ${NCURSES_SOURCE_DIR} ${NCURSES_OUTPUT_DIR}
    SOURCE_DIR "${NCURSES_SOURCE_DIR}"
    BINARY_DIR "${NCURSES_BUILD_DIR}"
    DEPENDS ncurses_extract_target
    CONFIGURE_COMMAND cd ${NCURSES_OUTPUT_DIR} && ./configure
        CC=${NCURSES_CC}
        --prefix=${NCURSES_INSTALL_DIR}
        CFLAGS=${NCURSES_CFLAGS}
        LDFLAGS=${NCURSES_LINKFLAGS}
        CPPFLAGS=${NCURSES_CPPFLAGS}
        --disable-root-environ
        --with-shared
        --without-normal
        --with-termlib
        --with-abi-version=5
        --with-versioned-syms
    USES_TERMINAL_BUILD TRUE
    BUILD_COMMAND cd ${NCURSES_OUTPUT_DIR} && make -j ${NPROC} install
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
    BUILD_ALWAYS OFF
)
set(NCURSES_LIB_DIR "${NCURSES_INSTALL_DIR}/lib" CACHE PATH "Ncurses library directory")
