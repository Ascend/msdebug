if(MS_DEBUGGER)
  lldb_tablegen(ProcessGDBRemoteProperties.inc -gen-lldb-property-defs
  SOURCE AscendProcessGDBRemoteProperties.td
  TARGET LLDBPluginProcessGDBRemotePropertiesGen)
else()
lldb_tablegen(ProcessGDBRemoteProperties.inc -gen-lldb-property-defs
  SOURCE ProcessGDBRemoteProperties.td
  TARGET LLDBPluginProcessGDBRemotePropertiesGen)
endif()

if(MS_DEBUGGER)
  lldb_tablegen(ProcessGDBRemotePropertiesEnum.inc -gen-lldb-property-enum-defs
  SOURCE AscendProcessGDBRemoteProperties.td
  TARGET LLDBPluginProcessGDBRemotePropertiesEnumGen)
else()
lldb_tablegen(ProcessGDBRemotePropertiesEnum.inc -gen-lldb-property-enum-defs
  SOURCE ProcessGDBRemoteProperties.td
  TARGET LLDBPluginProcessGDBRemotePropertiesEnumGen)
endif()

set(LLDB_PLUGINS
  lldbPluginProcessUtility
)

if(HAVE_LIBCOMPRESSION)
  set(LIBCOMPRESSION compression)
endif()

if(LLVM_ENABLE_ZLIB)
  list(APPEND LLDB_SYSTEM_LIBS ZLIB::ZLIB)
endif()

add_lldb_library(lldbPluginProcessGDBRemote PLUGIN
  GDBRemoteClientBase.cpp
  GDBRemoteCommunication.cpp
  GDBRemoteCommunicationClient.cpp
  GDBRemoteCommunicationHistory.cpp
  GDBRemoteCommunicationServer.cpp
  GDBRemoteCommunicationServerCommon.cpp
  GDBRemoteCommunicationServerLLGS.cpp
  GDBRemoteCommunicationServerPlatform.cpp
  GDBRemoteRegisterContext.cpp
  GDBRemoteRegisterFallback.cpp
  ProcessGDBRemote.cpp
  ProcessGDBRemoteLog.cpp
  ThreadGDBRemote.cpp

  LINK_LIBS
    lldbBreakpoint
    lldbCore
    lldbDataFormatters
    lldbHost
    lldbInterpreter
    lldbSymbol
    lldbTarget
    lldbUtility
    ${LLDB_SYSTEM_LIBS}
    ${LLDB_PLUGINS}
    ${LIBCOMPRESSION}
  LINK_COMPONENTS
    Support
    TargetParser
  )

add_dependencies(lldbPluginProcessGDBRemote
  LLDBPluginProcessGDBRemotePropertiesGen
  LLDBPluginProcessGDBRemotePropertiesEnumGen)
