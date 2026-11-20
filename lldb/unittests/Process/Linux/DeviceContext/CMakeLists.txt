add_subdirectory(fakelibs)

add_lldb_unittest(AclRuntimeTests
  AclRuntimeTest.cpp
  ${LLDB_SOURCE_DIR}/source/Plugins/Process/Linux/DeviceContext/AclRuntime.cpp
  LINK_LIBS lldbHost lldbUtility dl
  )

add_dependencies(AclRuntimeTests fake_acl_rt fake_acl_rt_missing)

target_include_directories(AclRuntimeTests PRIVATE
  ${LLDB_SOURCE_DIR}/source/Plugins/Process/Linux/DeviceContext)

target_compile_definitions(AclRuntimeTests PRIVATE MS_DEBUGGER=1
  FAKE_ACL_RT_LIB_PATH="${CMAKE_CURRENT_BINARY_DIR}/fakelibs/libacl_rt.so"
  FAKE_ACL_RT_MISSING_LIB_PATH="${CMAKE_CURRENT_BINARY_DIR}/fakelibs/libacl_rt_missing.so")
