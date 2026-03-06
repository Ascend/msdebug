find_program(PYTHON NAMES python3 python)
if(NOT PYTHON)
    message(FATAL_ERROR "Python not found! Required for packaging.")
endif()

set(VERSION_FILE "${ROOT_DIR}/package/conf/version.info")
if(NOT EXISTS "${VERSION_FILE}")
    message(FATAL_ERROR "Version info file not found: ${VERSION_FILE}")
endif()


set(PACKAGE_DIR "${ROOT_DIR}/package")

file(READ "${VERSION_FILE}" VERSION_CONTENT)
string(REGEX MATCH "Name=([^\n]+)" _ "${VERSION_CONTENT}")
set(PACKAGE_NAME "${CMAKE_MATCH_1}")
string(REGEX MATCH "Version=([^\n]+)" _ "${VERSION_CONTENT}")
set(PACKAGE_VERSION "${CMAKE_MATCH_1}")


set(OS_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
set(RUN_PACKAGE_NAME "${PACKAGE_NAME}_${PACKAGE_VERSION}_${OS_ARCH}.run")
set(MAKESELF_TOOL "${ROOT_DIR}/third-party/makeself/makeself.sh")
set(MAKESELF_HEADER "${ROOT_DIR}/third-party/makeself/makeself-header.sh")
set(HELP_INFO "${ROOT_DIR}/package/conf/help.info")
set(FILELIST_XML "${ROOT_DIR}/package/conf/filelist.xml")
set(INSTALL_SHELL share/info/${PACKAGE_NAME}/script/install.sh)
set(COMMENTS "ASCEND MINDSTUDIO DEBUGGER RUN PACKAGE")
file(GLOB_RECURSE PACKAGE_FILES
    "${PACKAGE_DIR}/*"
    "${PACKAGE_DIR}/**/*"
)

set(DEPENDS_FILES
    "${VERSION_FILE}"
    "${FILELIST_XML}"
    "${HELP_INFO}"
     ${PACKAGE_FILES}
)

file(GLOB_RECURSE MAKESELF_TAR_FILE_GLOB ${ROOT_DIR}/third-party/makeself/makeself-*.tar.gz)
file(GLOB_RECURSE MAKESELF_PATCH_FILE_GLOB ${ROOT_DIR}/third-party/makeself/makeself-*.patch)
list(GET MAKESELF_TAR_FILE_GLOB 0 MAKESELF_TAR_FILE)
list(GET MAKESELF_PATCH_FILE_GLOB 0 MAKESELF_PATCH_FILE)
message(STATUS "Use makeself tar file: ${MAKESELF_TAR_FILE}")
message(STATUS "Use makeself patch file: ${MAKESELF_PATCH_FILE}")

add_custom_command(
    OUTPUT ${MAKESELF_TOOL} ${MAKESELF_HEADER}
    COMMENT "Extract makeself component from ${MAKESELF_TAR_FILE}"
    # 跳过最外层目录，直接将内容解压到当前目录
    COMMAND tar xf ${MAKESELF_TAR_FILE} --strip-components=1
    COMMAND git apply ${MAKESELF_PATCH_FILE}
    # git 应用 patch 后不一定会更新akeself.sh 文件的时间戳，此处需要手动更新一下，
    # 保证 makeself 解压流程完成后重新触发打包
    COMMAND touch ${MAKESELF_TOOL} ${MAKESELF_HEADER}
    DEPENDS ${MAKESELF_TAR_FILE} ${MAKESELF_PATCH_FILE}
    WORKING_DIRECTORY ${ROOT_DIR}/third-party/makeself/
)

add_custom_command(
    OUTPUT "${CMAKE_INSTALL_PREFIX}/.package_done"
    COMMAND ${CMAKE_COMMAND} -E echo "Packaging Debugger - ${PACKAGE_NAME} v${PACKAGE_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}"
    COMMAND cd "${CMAKE_SOURCE_DIR}/package/script" && 
            ${PYTHON} parser.py -x "${FILELIST_XML}"
            --delivery_path "${CMAKE_INSTALL_PREFIX}" -o "${OS_ARCH}"
    COMMAND cd "${CMAKE_INSTALL_PREFIX}" && "${MAKESELF_TOOL}" --header "${MAKESELF_HEADER}" --help-header "${HELP_INFO}" --pigz --complevel 4 --nocrc --nomd5 --sha256 --chown --tar-quietly "${PACKAGE_NAME}" "${RUN_PACKAGE_NAME}" "${COMMENTS}" "${INSTALL_SHELL}"
    COMMAND chmod 750 ${CMAKE_INSTALL_PREFIX}/*.run
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_INSTALL_PREFIX}/.package_done"
    DEPENDS ${DEPENDS_FILES} ${MAKESELF_TOOL} ${MAKESELF_HEADER}
    COMMENT "Creating ${RUN_PACKAGE_NAME}"
)

add_custom_target(package_debugger ALL
    DEPENDS "${CMAKE_INSTALL_PREFIX}/.package_done"
)
set_property(TARGET package_debugger PROPERTY ALWAYS TRUE)
