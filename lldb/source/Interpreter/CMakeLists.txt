lldb_tablegen(InterpreterProperties.inc -gen-lldb-property-defs
  SOURCE InterpreterProperties.td
  TARGET LLDBInterpreterPropertiesGen)

lldb_tablegen(InterpreterPropertiesEnum.inc -gen-lldb-property-enum-defs
  SOURCE InterpreterProperties.td
  TARGET LLDBInterpreterPropertiesEnumGen)

set(LLDB_COMMAND_INTERPRETER_SRC
  CommandAlias.cpp
  CommandHistory.cpp
  CommandInterpreter.cpp
  CommandObject.cpp
  CommandOptionValidators.cpp
  CommandReturnObject.cpp
  OptionArgParser.cpp
  OptionGroupArchitecture.cpp
  OptionGroupBoolean.cpp
  OptionGroupFile.cpp
  OptionGroupFormat.cpp
  OptionGroupMemoryTag.cpp
  OptionGroupPythonClassWithDict.cpp
  OptionGroupOutputFile.cpp
  OptionGroupPlatform.cpp
  OptionGroupString.cpp
  OptionGroupUInt64.cpp
  OptionGroupUUID.cpp
  OptionGroupValueObjectDisplay.cpp
  OptionValue.cpp
  OptionValueArch.cpp
  OptionValueArgs.cpp
  OptionValueArray.cpp
  OptionValueBoolean.cpp
  OptionValueChar.cpp
  OptionValueDictionary.cpp
  OptionValueEnumeration.cpp
  OptionValueFileColonLine.cpp
  OptionValueFileSpec.cpp
  OptionValueFileSpecList.cpp
  OptionValueFormat.cpp
  OptionValueFormatEntity.cpp
  OptionValueLanguage.cpp
  OptionValuePathMappings.cpp
  OptionValueProperties.cpp
  OptionValueRegex.cpp
  OptionValueSInt64.cpp
  OptionValueString.cpp
  OptionValueUInt64.cpp
  OptionValueUUID.cpp
  OptionGroupVariable.cpp
  OptionGroupWatchpoint.cpp
  Options.cpp
  Property.cpp
  ScriptInterpreter.cpp
)
if(MS_DEBUGGER)
  list(APPEND LLDB_COMMAND_INTERPRETER_SRC OptionValueMemoryType.cpp)
endif()
add_lldb_library(lldbInterpreter NO_PLUGIN_DEPENDENCIES
  ${LLDB_COMMAND_INTERPRETER_SRC}

  LINK_LIBS
    lldbCommands
    lldbCore
    lldbDataFormatters
    lldbHost
    lldbTarget
    lldbUtility

  LINK_COMPONENTS
    Support
  )

add_dependencies(lldbInterpreter
  LLDBInterpreterPropertiesGen
  LLDBInterpreterPropertiesEnumGen)

