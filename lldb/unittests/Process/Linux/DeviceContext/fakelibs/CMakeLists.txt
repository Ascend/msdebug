set(FAKE_ACL_RT_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Fake libacl_rt.so with all public ACL entry points.
add_library(fake_acl_rt SHARED FakeAclRuntimeLib.cpp)
set_target_properties(fake_acl_rt PROPERTIES
  OUTPUT_NAME acl_rt
  LIBRARY_OUTPUT_DIRECTORY ${FAKE_ACL_RT_OUTPUT_DIR})

# Same fake library but without aclrtMemcpy, to exercise the
# "required symbol missing => Initialize() fails" path.
add_library(fake_acl_rt_missing SHARED FakeAclRuntimeLib.cpp)
set_target_properties(fake_acl_rt_missing PROPERTIES
  OUTPUT_NAME acl_rt_missing
  LIBRARY_OUTPUT_DIRECTORY ${FAKE_ACL_RT_OUTPUT_DIR})
target_compile_definitions(fake_acl_rt_missing PRIVATE FAKE_SKIP_ACLRT_MEMCPY=1)
