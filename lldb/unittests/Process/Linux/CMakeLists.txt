if(MS_DEBUGGER)
  add_lldb_unittest(ProcessLinuxTests
    PerfTests.cpp
    ProcfsTests.cpp
    AscendProcessLinuxTest.cpp
    LINK_LIBS
      LLVMTestingSupport
      lldbPluginProcessLinux
      lldbPluginProcessGDBRemote
      -Wl,--wrap=ioctl
      -Wl,--wrap=open
    )
else()
  add_lldb_unittest(ProcessLinuxTests
    PerfTests.cpp
    ProcfsTests.cpp
    LINK_LIBS
      lldbPluginProcessLinux
    )
endif()

target_include_directories(ProcessLinuxTests PRIVATE
  ${LLDB_SOURCE_DIR}/source/Plugins/Process/Linux)
