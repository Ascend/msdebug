/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER

#include "Ascend310PDeviceContext.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/RegisterValue.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend310P.h"
using namespace lldb_private;
using namespace lldb;

Ascend310PDeviceContext::Ascend310PDeviceContext(const ::pid_t pid, const uint32_t device_id):
  DeviceContext(pid, device_id) {
  m_soc_type = SocType::ASCEND310P;
}

Status Ascend310PDeviceContext::ReadRegister(const RegisterInfo *reg_info,
                                             uint32_t core_id, CoreType core_type, RegisterValue &value) {
  Status error;
    const auto &register_map = RegisterInfoPOSIX_ascend310P::GetRegExtractor().register_map;
    if (reg_info && reg_info->kinds[eRegisterKindLLDB] < register_map.size() &&
      register_map.find(reg_info->name) != register_map.end()) {
    const DeviceRegisterInfo &device_reg_info = register_map.at(reg_info->name);
    uint64_t addr = device_reg_info.addr;
    uint64_t value_int;
    error = DeviceContext::ReadRegister(addr, core_id, core_type, value_int);
    if (error.Fail()) {
      return error;
    }
    value.SetUInt64(value_int);
  } else {
    error.SetErrorString("reg_info is null or reg_num in reg_info is invalid");
  }
  return error;
}

Status Ascend310PDeviceContext::GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend310P::GetRegExtractor().register_map;
  auto reg_info = register_map.find(reg_name.str());
  if (reg_info == register_map.end()) {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register name: {0}", reg_name);
    return error;
  }
  addr = reg_info->second.addr;
  return error;
}

Status Ascend310PDeviceContext::GetRegisterList(std::vector<std::string> &reg_list, CoreType core_type) {
  Status error;
  if (core_type == CoreType::UNKNOWN_CORE_TYPE) {
    error.SetErrorString("GetRegisterList failed due to unknown core type.");
    return error;
  }
  const auto &register_map = RegisterInfoPOSIX_ascend310P::GetRegExtractor().register_map;
  for(const auto &item : register_map) {
    reg_list.push_back(item.first);
  }
  return error;
}

Status Ascend310PDeviceContext::CheckRegisterAddr(CoreType core_type, uint64_t addr) {
  Status error;
  const auto &register_map = RegisterInfoPOSIX_ascend310P::GetRegExtractor().register_map;
  for (const auto &item : register_map) {
    if (item.second.addr == addr) {
      return error;
    }
  }
  error.SetErrorStringWithFormatv("CheckRegisterAddr failed due to error register addr, addr={0:x}, core_type={1}",
                                  addr, static_cast<uint8_t>(core_type));
  return error;
}

MemType Ascend310PDeviceContext::GetStackMemType() const {
  return MemType::SCALAR_BUF;
}

Status Ascend310PDeviceContext::Resume(const InterruptPosInfo &pos_info) const {
  // 310P supports only the resume of all cores
  return BaseSqCqComm(CmdType::RESUME_DEVICE);
}

inline uint64_t GetOffsetFor32Bit(uint64_t m, uint64_t n) {
  constexpr uint8_t element_size = 4;
  constexpr uint8_t col_num = 4;
  // 行偏移量 + 列偏移量
  return m * col_num * element_size + n % col_num * element_size;
}

inline uint64_t GetOffsetFor16Bit(uint64_t m, uint64_t n) {
  constexpr uint8_t big_block_row = 16;
  constexpr uint8_t small_block_row = 4;
  constexpr uint8_t col_num = 4;
  constexpr uint8_t element_size = 2;
  uint64_t block_256_id = m / big_block_row;
  // 两个块的偏移:
  uint64_t offset = block_256_id / 2 * 2 * big_block_row * col_num * element_size;
  // 块内行偏移量 + 相邻块行偏移量
  offset += m % big_block_row / small_block_row * 2 * small_block_row * col_num * element_size;
  if (block_256_id % 2 == 1) {
    offset += small_block_row * col_num * element_size;
  }
  // 块内剩余行偏移量 + 列偏移量
  offset += m % small_block_row * col_num * element_size + n % col_num * element_size;
  return offset;
}

size_t Ascend310PDeviceContext::ReadL0ABMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                               const InterruptPosInfo &pos_info, void *data) {
  size_t left_size = size;
  uint8_t *out = static_cast<uint8_t*>(data);
  addr *= 2;
  while(left_size > 0) {
    auto read_size = DeviceContext::ReadLocalMemory(addr, 1, memory_type_info, pos_info, out);
    if (read_size == 0) {
      return 0;
    }
    left_size -= read_size;
    out += read_size;
    // 不读奇数地址，无效内容
    addr += 2;
  }
  return size;
}

size_t Ascend310PDeviceContext::ReadL0CMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                              const InterruptPosInfo &pos_info, void *data) {
  Log *log = GetLog(LLDBLog::Process);
  auto element_size = memory_type_info.element_size;
  size_t left_size = size;
  uint8_t *out = static_cast<uint8_t*>(data);
  while(left_size > 0) {
    uint64_t origin_m = addr / element_size / 16;
    uint64_t origin_n = addr / element_size % 16;
    uint64_t block_begin_addr = origin_n / 4 * 0x10000;
    uint64_t convert_addr{0};
    uint64_t expect_size = 0;
    if (element_size == 4) {
      auto offset = GetOffsetFor32Bit(origin_m, origin_n);
      expect_size = std::min(left_size, (offset / 16 + 1) * 16 - offset);
      convert_addr = block_begin_addr + offset;
    } else {
      auto offset = GetOffsetFor16Bit(origin_m, origin_n);
      expect_size = std::min(left_size, (offset / 8 + 1) * 8 - offset);
      convert_addr = block_begin_addr + GetOffsetFor16Bit(origin_m, origin_n);
    }
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

size_t Ascend310PDeviceContext::ReadLocalMemory(
    lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
    const InterruptPosInfo &pos_info, void *data) {
  Log *log = GetLog(LLDBLog::Process);
  auto mem_type = DeviceAddressClassToMemType(memory_type_info.address_class);
  LLDB_LOG(log, "{0} read local memory: element_size={1}, mem_type={2}",
           __FUNCTION__, memory_type_info.element_size, static_cast<uint32_t>(mem_type));
 
  if (mem_type == MemType::L0A || mem_type == MemType::L0B) {
    return ReadL0ABMemory(addr, size, memory_type_info, pos_info, data);
  } else if (mem_type != MemType::L0C ||
      (memory_type_info.element_size != 2 && memory_type_info.element_size != 4)) {
    return DeviceContext::ReadLocalMemory(addr, size, memory_type_info, pos_info, data);
  } else {
    return ReadL0CMemory(addr, size, memory_type_info, pos_info, data);
  }
  return size;
}

Status Ascend310PDeviceContext::InvalidInstrCache(const addr_t &addr,
  const InterruptPosInfo &pos_info, uint8_t redirect_ifu) const {
  // because 310P supports only the resume of all cores,
  // the INVALID_INSTR_CACHE must use all cores
  Status error;
  InvalidCacheParam param;
  param.enable_all = 0;
  param.virt_addr = addr;
  param.redirect_ifu = redirect_ifu;
  error = BaseSqCqComm(CmdType::INVALID_INSTR_CACHE, (uint8_t*)&param, sizeof(param));
  if (error.Fail()) {
    return error;
  }
  return error;
}
#endif // ifdef MS_DEBUGGER
