# 根据构建环境信息生成scene.info
string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM)
set(ENV_INFO "os=${SYSTEM}\narch=${CMAKE_SYSTEM_PROCESSOR}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/scene.info" "${ENV_INFO}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/scene.info"
                "${CMAKE_SOURCE_DIR}/package/conf/version.info"
                DESTINATION ${CMAKE_SOURCE_DIR}/output)
