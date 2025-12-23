set(LLDB_DISASSEMBLER_SRC
  DisassemblerLLVMC.cpp
  )
if(MS_DEBUGGER)
    list(APPEND LLDB_DISASSEMBLER_SRC AscendDisassemblerHelper.cpp)
endif()
add_lldb_library(lldbPluginDisassemblerLLVMC PLUGIN
  ${LLDB_DISASSEMBLER_SRC}

  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
  LINK_COMPONENTS
    ${LLVM_TARGETS_TO_BUILD}
    MC
    MCDisassembler
    RuntimeDyld
    Support
    TargetParser
  )
