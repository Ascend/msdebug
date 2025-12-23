/*
* Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
*/

#ifdef MS_DEBUGGER
#include "lldb/Interpreter/OptionValueMemoryType.h"

#include "lldb/DataFormatters/FormatManager.h"
#include "lldb/Target/Memory.h"
#include "lldb/Utility/Stream.h"

using namespace lldb;
using namespace lldb_private;

void OptionValueMemoryType::DumpValue(const ExecutionContext *exe_ctx, 
                                      Stream &strm, uint32_t dump_mask) {
  if (dump_mask & eDumpOptionType)
    strm.Printf("(%s)", GetTypeAsCString());
  if (dump_mask & eDumpOptionValue) {
    if (dump_mask & eDumpOptionType)
      strm.PutCString(" = ");
    if (m_current_value != DeviceAddressClass::NONE)
      strm.PutCString(MemoryType::GetNameForMemoryType(m_current_value));
  }
}

Status OptionValueMemoryType::SetValueFromString(llvm::StringRef value, 
                                                 VarSetOperationType op) {
  Status error;
  switch (op) {
  case eVarSetOperationClear:
    Clear();
    break;

  case eVarSetOperationReplace:
  case eVarSetOperationAssign: {
    ConstString mem_type_name(value.trim());
    DeviceAddressClass mem_type = MemoryType::GetMemoryTypeFromString(mem_type_name.GetStringRef());
    if (mem_type != DeviceAddressClass::NONE) {
      m_value_was_set = true;
      m_current_value = mem_type;
    } else {
      StreamString error_strm;
      error_strm.Printf("invalid memory type '%s'.\n", value.str().c_str());
      error.SetErrorString(error_strm.GetString());
    }
  } break;

  case eVarSetOperationInsertBefore:
  case eVarSetOperationInsertAfter:
  case eVarSetOperationRemove:
  case eVarSetOperationAppend:
  case eVarSetOperationInvalid:
    error = OptionValue::SetValueFromString(value, op);
    break;
  }
  return error;
}
#endif
