add_subdirectory(argdumper)
add_subdirectory(driver)
add_subdirectory(intel-features)

# We want lldb-test to be built only when it's needed,
# i.e. if a target requires it as dependency. The typical
# example is `check-lldb`. So, we pass EXCLUDE_FROM_ALL here.
add_subdirectory(lldb-test EXCLUDE_FROM_ALL)
add_subdirectory(lldb-fuzzer EXCLUDE_FROM_ALL)

add_lldb_tool_subdirectory(lldb-instr)
add_lldb_tool_subdirectory(lldb-dap)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_lldb_tool_subdirectory(darwin-debug)
  if(NOT LLDB_USE_SYSTEM_DEBUGSERVER)
    add_lldb_tool_subdirectory(debugserver)
  endif()
endif()

if (LLDB_CAN_USE_LLDB_SERVER)
  add_lldb_tool_subdirectory(lldb-server)
endif()

if (MS_DEBUGGER)
  add_subdirectory(msdebug)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} _sys)
  file(GENERATE OUTPUT "${LLVM_BINARY_DIR}/scene.info"
      CONTENT "os=${_sys}\narch=${CMAKE_SYSTEM_PROCESSOR}\n")

  install(FILES "${ROOT_DIR}/package/conf/version.info"
          DESTINATION "."
          COMPONENT msdebug)
  
  install(FILES "${LLVM_BINARY_DIR}/scene.info"
          DESTINATION "."
          COMPONENT msdebug)
  
  install(PROGRAMS "${ROOT_DIR}/lldb/tools/msdebug/msdebug_script.sh"
          DESTINATION bin
          RENAME msdebug
          COMPONENT msdebug)

  add_custom_target(_msdebug_libs_copy
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/lib"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${LIBEDIT_LIBRARY}/libedit.so.0"
          "${CMAKE_INSTALL_PREFIX}/lib/libedit.so.0"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${NCURSES_LIB_DIR}/libncurses.so.5"
          "${CMAKE_INSTALL_PREFIX}/lib/libncurses.so.5"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${NCURSES_LIB_DIR}/libtinfo.so.5"
          "${CMAKE_INSTALL_PREFIX}/lib/libtinfo.so.5"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${NCURSES_LIB_DIR}/libpanel.so.5"
          "${CMAKE_INSTALL_PREFIX}/lib/libpanel.so.5"
      COMMENT "Copying library files without symlinks"
      VERBATIM
  )

  add_custom_target(install-msdebug-extra
      COMMAND ${CMAKE_COMMAND} -E rename
          "${CMAKE_INSTALL_PREFIX}/bin/lldb"
          "${CMAKE_INSTALL_PREFIX}/bin/msdebug.bin"
      COMMENT "Renaming lldb binary"
      DEPENDS install-lldb install-liblldb install-lldb-server install-runtime_stub _msdebug_libs_copy
  )

  add_custom_target(install-msdebug
      COMMAND ${CMAKE_COMMAND} --install . --component msdebug
      COMMENT "Finalizing msdebug installation"
      DEPENDS install-msdebug-extra
  )
endif()

