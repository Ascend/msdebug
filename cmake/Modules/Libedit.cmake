
include(ExternalProject)
include(ProcessorCount)

set(LIBEDIT_LDFLAGS "-L${NCURSES_INSTALL_DIR}/lib -Wl,-z,now -Wl,-s")
set(LIBEDIT_CPPFLAGS "-I${NCURSES_INSTALL_DIR}/include -I${NCURSES_INSTALL_DIR}/include/ncurses -I${ROOT_DIR}/third-party/ncurses -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O2 -ftrapv")



set(LIBEDIT_TAR_GZ "${ROOT_DIR}/third-party/libedit/libedit-20230828-3.1.tar.gz")
set(LIBEDIT_INSTALL_DIR "${PROJECT_BUILD_DIR}/libedit" CACHE PATH "Libedit install directory")
ExternalProject_Add(libedit_project
    DOWNLOAD_COMMAND tar -xzf ${LIBEDIT_TAR_GZ} -C <BINARY_DIR>
    CONFIGURE_COMMAND cd <BINARY_DIR>/libedit-20230828-3.1 && ./configure
        LDFLAGS=${LIBEDIT_LDFLAGS}
        CPPFLAGS=${LIBEDIT_CPPFLAGS}
        --prefix=${LIBEDIT_INSTALL_DIR}
    USES_TERMINAL_BUILD TURE
    BUILD_COMMAND cd <BINARY_DIR>/libedit-20230828-3.1 && make -j ${NPROC} install
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
    BUILD_ALWAYS OFF
    DEPENDS ncurses_project
)

ExternalProject_Get_Property(libedit_project BINARY_DIR)
set(LIBEDIT_SRC_DIR "${BINARY_DIR}/libedit-20230828-3.1" CACHE PATH "LibEdit SRC directory")

set(LIBEDIT_LIBRARY "${LIBEDIT_INSTALL_DIR}/lib" 
    CACHE INTERNAL "Libedit library file"
)