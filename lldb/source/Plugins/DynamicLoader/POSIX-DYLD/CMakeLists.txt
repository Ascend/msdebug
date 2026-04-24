if (MS_DEBUGGER)
add_lldb_library(lldbPluginDynamicLoaderPosixDYLD PLUGIN
  DYLDRendezvous.cpp
  DynamicLoaderPOSIXDYLD.cpp
  InstructionMatcher.cpp

  LINK_LIBS
    lldbBreakpoint
    lldbCore
    lldbHost
    lldbSymbol
    lldbTarget
    lldbPluginProcessElfCore
    lldbPluginProcessUtility
  LINK_COMPONENTS
    Support
  )
else()
add_lldb_library(lldbPluginDynamicLoaderPosixDYLD PLUGIN
  DYLDRendezvous.cpp
  DynamicLoaderPOSIXDYLD.cpp

  LINK_LIBS
    lldbBreakpoint
    lldbCore
    lldbHost
    lldbSymbol
    lldbTarget
    lldbPluginProcessElfCore
    lldbPluginProcessUtility
  LINK_COMPONENTS
    Support
  )
endif()
