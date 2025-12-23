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

ExternalProject_Add(ncurses_project
    DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory ${NCURSES_SOURCE_DIR} ${NCURSES_OUTPUT_DIR}
    SOURCE_DIR "${NCURSES_SOURCE_DIR}"
    BINARY_DIR "${NCURSES_BUILD_DIR}"
    CONFIGURE_COMMAND cd ${NCURSES_OUTPUT_DIR} && ./configure
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
