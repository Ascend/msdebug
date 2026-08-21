add_lldb_unittest(ElfSymbolCheckTests
  ElfSymbolCheckTest.cpp
  )

target_include_directories(ElfSymbolCheckTests PRIVATE
  ${LLDB_SOURCE_DIR}/tools/msdebug)
