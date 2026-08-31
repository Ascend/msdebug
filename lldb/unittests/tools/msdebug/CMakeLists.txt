add_lldb_unittest(ElfSymbolCheckTests
  ElfSymbolCheckTest.cpp
  )

add_subdirectory(aclrt_stub_smoke)

target_include_directories(ElfSymbolCheckTests PRIVATE
  ${LLDB_SOURCE_DIR}/tools/msdebug)
