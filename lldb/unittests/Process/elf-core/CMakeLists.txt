if(MS_DEBUGGER)
add_lldb_unittest(ProcessElfCoreDeviceTests
  ProcessElfCoreDeviceTest.cpp

  LINK_LIBS
    lldbPluginProcessElfCoreDevice
    lldbCore
    lldbHost
    lldbTarget
    lldbUtility
    lldbUtilityHelpers
    lldbInterpreter
    lldbPluginPlatformLinux
    LLVMTestingSupport
)
endif()
