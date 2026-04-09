if (MS_DEBUGGER)
  find_package(LLVM)
  add_library(runtime_stub SHARED
      runtime_stub.cpp
      get_tiling_key.cpp
      AscendCommunicationClient.cpp
      aclrt_stub.cpp
      HijackedLayerManager.cpp
  )
  target_include_directories(runtime_stub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
  target_link_libraries(runtime_stub dl LLVMSupport LLVMObject lldbHost)
  target_compile_options(runtime_stub PRIVATE -fvisibility=hidden)
  target_link_options(runtime_stub PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/version.map")

  add_custom_target(install-runtime_stub
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/lib"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:runtime_stub>"
          "${CMAKE_INSTALL_PREFIX}/lib/libruntime_stub.so" 
      COMMENT "Installing runtime_stub.so to ${CMAKE_INSTALL_PREFIX}/lib/"
      DEPENDS runtime_stub
  )
endif()
