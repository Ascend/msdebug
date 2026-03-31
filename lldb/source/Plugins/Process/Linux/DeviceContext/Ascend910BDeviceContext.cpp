/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include <map>

#include "Ascend910BDeviceContext.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/RegisterValue.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend910B.h"


using namespace lldb_private;
using namespace lldb;

Ascend910BDeviceContext::Ascend910BDeviceContext(const ::pid_t pid, const uint32_t device_id):
  DeviceContext(pid, device_id) {
  m_soc_type = SocType::ASCEND910B;
}

Status Ascend910BDeviceContext::ReadRegister(const RegisterInfo *reg_info,
                                             const InterruptPosInfo &pos_info, RegisterValue &value) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910B::GetRegExtractor().register_map;
  if (reg_info && reg_info->kinds[eRegisterKindLLDB] < register_map.size() &&
      register_map.find(reg_info->name) != register_map.end()) {
    const DeviceRegisterInfo &device_reg_info = register_map.at(reg_info->name);
    uint64_t addr = device_reg_info.addr;
    error = DeviceContext::ReadRegister(addr, reg_info, pos_info.core_id, pos_info.core_type, value);
    if (error.Fail()) {
      return error;
    }
  } else {
    error.SetErrorString("reg_info is null or reg_num in reg_info is invalid");
  }
  return error;
}


Status Ascend910BDeviceContext::GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910B::GetRegExtractor().register_map;
  auto reg_info = register_map.find(reg_name.str());
  if (reg_info == register_map.end()) {
      error.SetErrorStringWithFormatv(
          "Can not get addr, register name: {0}", reg_name);
      return error;
  }
  if ((1U << static_cast<int>(core_type)) & reg_info->second.core_type_support_mask) {
    addr = reg_info->second.addr;
  } else {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register {0} is not support", reg_name);
  }
  return error;
}

Status Ascend910BDeviceContext::GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) {
  Status error;
  if (core_type == CoreType::UNKNOWN_CORE_TYPE) {
    error.SetErrorString("GetRegisterList failed due to unknown core type.");
    return error;
  }
  const auto &register_map = RegisterInfoPOSIX_ascend910B::GetRegExtractor().register_map;
  for(const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      reg_list.push_back(item.first);
    }
  }
  return error;
}

Status Ascend910BDeviceContext::CheckRegisterAddr(CoreType core_type, uint64_t addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend910B::GetRegExtractor().register_map;
  for (const auto &item : register_map) {
    if ((1U << static_cast<int>(core_type)) & item.second.core_type_support_mask) {
      if (item.second.addr == addr) {
        return error;
      }
    }
  }
  error.SetErrorString("CheckRegisterAddr failed due to error register addr");
  return error;
}

MemType Ascend910BDeviceContext::GetStackMemType() const {
  return MemType::OUT_MEM;
}

size_t Ascend910BDeviceContext::ReadLocalMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                 const InterruptPosInfo &pos_info, void *data) {
  auto mem_type = DeviceAddressClassToMemType(memory_type_info.address_class);
  if (mem_type != MemType::L0C) {
    return DeviceContext::ReadLocalMemory(addr, size, memory_type_info, pos_info, data);
  }
  constexpr uint32_t element_size = 4;
  Log *log = GetLog(LLDBLog::Process);
  size_t left_size = size;
  uint8_t *out = static_cast<uint8_t*>(data);
  while(left_size > 0) {
    uint64_t origin_m = addr / element_size / 16;
    uint64_t origin_n = addr / element_size % 16;
    uint64_t offset = origin_m / 2 * 32 * element_size;
    offset += origin_n / 4 * (8 * element_size);
    offset += origin_m % 2 == 1 ? 4 * element_size : 0;
    offset += origin_n % 4 * element_size;
    auto convert_addr = offset;
    auto expect_size = std::min(left_size, (offset / 16 + 1) * 16 - offset);
    LLDB_LOG(log, "{0} read local memory: origin addr={1:x}, convert_addr={2:x}, expect_size={3}",
             __FUNCTION__, addr, convert_addr, expect_size);
    auto read_size = DeviceContext::ReadLocalMemory(convert_addr, expect_size, memory_type_info, pos_info, out);
    if (read_size == 0) {
      return 0;
    }
    left_size -= expect_size;
    addr += expect_size;
    out += expect_size;
  }
  return size;
}
#endif // ifdef MS_DEBUGGER
