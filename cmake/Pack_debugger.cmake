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
set(RUN_PACKAGE_NAME "Ascend-${PACKAGE_NAME}-${PACKAGE_VERSION}_linux-${OS_ARCH}.run")
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
    DEPENDS ${DEPENDS_FILES}
    COMMENT "Creating ${RUN_PACKAGE_NAME}"
)



add_custom_target(package_debugger ALL
    DEPENDS "${CMAKE_INSTALL_PREFIX}/.package_done"
)
set_property(TARGET package_debugger PROPERTY ALWAYS TRUE)
