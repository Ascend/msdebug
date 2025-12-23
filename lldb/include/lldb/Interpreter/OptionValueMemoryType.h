/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef LLDB_INTERPRETER_OPTIONVALUEMEMORYTYPE_H
#define LLDB_INTERPRETER_OPTIONVALUEMEMORYTYPE_H

#include "lldb/Interpreter/OptionValue.h"
#include "lldb/lldb-enumerations.h"
#include "lldb/Utility/ArchSpec.h"

namespace lldb_private {

class OptionValueMemoryType : public Cloneable<OptionValueMemoryType, OptionValue> {
public:
    OptionValueMemoryType(DeviceAddressClass value) : m_current_value(value), m_default_value(value) {}

    OptionValueMemoryType(DeviceAddressClass current_value, DeviceAddressClass default_value)
        : m_current_value(current_value),
          m_default_value(default_value) {}

  ~OptionValueMemoryType() override = default;

  // Virtual subclass pure virtual overrides

  OptionValue::Type GetType() const override { return eTypeMemoryType; }

  void DumpValue(const ExecutionContext *exe_ctx, Stream &strm,
                 uint32_t dump_mask) override;

  Status
  SetValueFromString(llvm::StringRef value,
                     VarSetOperationType op = eVarSetOperationAssign) override;

  void Clear() override {
    m_current_value = m_default_value;
    m_value_was_set = false;
  }

  // Subclass specific functions

  DeviceAddressClass GetCurrentValue() const { return m_current_value; }

  DeviceAddressClass GetDefaultValue() const { return m_default_value; }

  void SetCurrentValue(DeviceAddressClass value) { m_current_value = value; }

  void SetDefaultValue(DeviceAddressClass value) { m_default_value = value; }

protected:
  DeviceAddressClass m_current_value;
  DeviceAddressClass m_default_value;
};

#ifdef MS_DEBUGGER
struct MemoryReaderParamClient {
    ArchSpec arch_spec{};
    DeviceAddressClass address_class{DeviceAddressClass::NONE};
    // 310P读L0C时需要知道L0C的指针类型决定映射公式
    uint8_t element_size{0};
};

// server侧读取内存时的参数信息
// 因为从client只拿到了arch_type, 所以要新建1个结构体
struct MemoryReaderParamForServer {
    llvm::Triple::ArchType arch_type{llvm::Triple::UnknownArch};
    DeviceAddressClass address_class{DeviceAddressClass::NONE};
    // 310P读L0C时需要知道L0C的指针类型决定映射公式
    uint8_t element_size{0};
};
#endif

} // namespace lldb_private

#endif // LLDB_INTERPRETER_OPTIONVALUEMEMORYTYPE_H
#endif
