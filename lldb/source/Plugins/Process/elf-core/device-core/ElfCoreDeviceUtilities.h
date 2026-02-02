/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER

#ifndef LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_ELFCOREDEVICEUTILITIES_H
#define LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_ELFCOREDEVICEUTILITIES_H

#include "lldb/Utility/MessageDefines.h"

#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <vector>

using CoreIDType = uint16_t;
namespace lldb_private {
namespace device_core {
enum class GlobalDataType : uint16_t {
  INVALID_TENSOR = 0,
  GENERAL_TENSOR = 1,
  INPUT_TENSOR = 2,
  OUTPUT_TENSOR = 3,
  WORKSPACE_TENSOR = 4,
  MC2_CTX = 6,
  TILING_DATA = 7,
  SHAPE_TENSOR = 12,
  ARGS = 101,
  STACK = 102,
  DEVICE_KERNEL_OBJECT = 103,   // device侧GM中算子.o数据
  MAX
};

enum class DevdrvChipType : uint32_t {
  CHIP_BEGIN = 0,
  CHIP_MINI = CHIP_BEGIN,
  CHIP_CLOUD,
  CHIP_MDC,
  CHIP_LHISI,
  CHIP_DC,
  CHIP_CLOUD_V2 = 5,  // 910B/C
  CHIP_RESERVED = 6,
  CHIP_MINI_V3 = 7,
  CHIP_TINY_V1 = 8,
  CHIP_NANO_V1 = 9,
  CHIP_KUNPENG920F = 10,
  CHIP_AS31XM1 = 11,
  CHIP_610LITE = 12,
  CHIP_CLOUD_V3 = 13,
  CHIP_CLOUD_V4 = 15, // david
  CHIP_END
};

struct GlobalMemInfo {
  uint64_t addr; // 虚拟地址
  uint64_t size;  // 内存大小
  uint32_t section_index; // 对应哪个.ascend.global section
  GlobalDataType type; // 内存是input/output/workspace/stack等类型
  uint16_t reserve;
  union {
    struct {
      CoreIDType coreId;
    } coreInfo;                // stack 类型的内存区分不同core
    struct {
      uint32_t dim; // tensor shape
      uint32_t reserve;
      uint64_t dim_size[25];
    } shape;                    // input、output
  };
};

struct LocalMemInfo {
  uint64_t size; //内存大小
  uint32_t section_index; // 对应哪个.ascend.local section
  uint32_t global_section_index; //对应哪个.ascend.gloabl section， 只有dcache有效
  MemType type; // L0AL1等
  uint32_t reserve;
};

struct RegDataBase {
  uint64_t addr;
  uint8_t invalid;
  uint8_t reserve[6];
  uint8_t reg_size;
  virtual ~RegDataBase() = default;
  virtual const uint8_t* GetValue() const = 0;
  virtual uint8_t GetValueSize() const = 0;
};

struct RegDataV2 : RegDataBase {
  uint8_t value[16];
  const uint8_t* GetValue() const override {return value;}
  uint8_t GetValueSize() const override {return 16;}
};

struct RegDataV4 : RegDataBase {
  uint8_t value[32];
  const uint8_t* GetValue() const override {return value;}
  uint8_t GetValueSize() const override {return 32;}
};

struct GlobalMemory {
  GlobalMemInfo global_mem_info;
  bool valid;
  std::vector<uint8_t> data;
};

struct LocalMemory {
  LocalMemInfo local_mem_info;
  uint64_t addr; // DCACHE
  bool valid;
  std::vector<uint8_t> data;
};

struct SummaryInfo {
  std::vector<CoreIDType> aiv_id;
  std::vector<CoreIDType> aic_id;
  uint32_t dev_id;
  DevdrvChipType chip_type = DevdrvChipType::CHIP_BEGIN;
  std::unordered_map<GlobalDataType, std::vector<GlobalMemory>> global_mems;
  // map key is section_id
  std::unordered_map<uint64_t, GlobalMemory> global_section_to_memory;
  // map key is core_id, mem_type
  std::unordered_map<CoreIDType, std::unordered_map<lldb_private::MemType, std::vector<LocalMemory>>> local_mems;
  // map key is core_id, addr
  std::unordered_map<CoreIDType, std::unordered_map<uint64_t, std::unique_ptr<RegDataBase>>> reg_data;
  CoreIDType focus_core_id;
  CoreType focus_core_type;
  uint64_t base_pc;
};

static std::unordered_map<DevdrvChipType, std::string> DevdrvChipTypeToStr {
  {DevdrvChipType::CHIP_CLOUD_V2, "A2/A3"},
  {DevdrvChipType::CHIP_CLOUD_V4, "A5"}
};

inline CoreIDType GetMaxAicID(DevdrvChipType chip_type) {
  const static std::unordered_map<DevdrvChipType, CoreIDType> max_aic_core_id_table {
    {DevdrvChipType::CHIP_CLOUD_V2, 25},
    {DevdrvChipType::CHIP_CLOUD_V4, 36}
  };
  if (max_aic_core_id_table.find(chip_type) == max_aic_core_id_table.end()) {
    return 0;
  }
  return max_aic_core_id_table.at(chip_type);
}

static std::unordered_map<GlobalDataType, std::string> GlobalDataTypeToStr {
  {GlobalDataType::INVALID_TENSOR, "INVALID_TENSOR"},
  {GlobalDataType::GENERAL_TENSOR, "GENERAL_TENSOR"},
  {GlobalDataType::INPUT_TENSOR, "INPUT_TENSOR"},
  {GlobalDataType::OUTPUT_TENSOR, "OUTPUT_TENSOR"},
  {GlobalDataType::WORKSPACE_TENSOR, "WORKSPACE_TENSOR"},
  {GlobalDataType::MC2_CTX, "MC2_CTX"},
  {GlobalDataType::TILING_DATA, "TILING_DATA"},
  {GlobalDataType::SHAPE_TENSOR, "SHAPE_TENSOR"},
  {GlobalDataType::ARGS, "ARGS"},
  {GlobalDataType::STACK, "STACK"},
  {GlobalDataType::DEVICE_KERNEL_OBJECT, "DEVICE_KERNEL_OBJECT"},
  {GlobalDataType::MAX, "NULL"}
};

static std::unordered_map<GlobalDataType, std::string> GlobalDataTypeToMemTypeStr {
  {GlobalDataType::INVALID_TENSOR, "GM"},
  {GlobalDataType::GENERAL_TENSOR, "GM"},
  {GlobalDataType::INPUT_TENSOR, "GM"},
  {GlobalDataType::OUTPUT_TENSOR, "GM"},
  {GlobalDataType::WORKSPACE_TENSOR, "GM"},
  {GlobalDataType::MC2_CTX, "GM"},
  {GlobalDataType::TILING_DATA, "GM/DCACHE"},
  {GlobalDataType::SHAPE_TENSOR, "GM"},
  {GlobalDataType::ARGS, "GM/DCACHE"},
  {GlobalDataType::STACK, "GM/DCACHE"},
  {GlobalDataType::DEVICE_KERNEL_OBJECT, "GM"}
};

static std::unordered_map<MemType, std::string> LocalDataTypeToStr {
  {lldb_private::MemType::L0A, "L0A"},
  {lldb_private::MemType::L0B, "L0B"},
  {lldb_private::MemType::L0C, "L0C"},
  {lldb_private::MemType::UB, "UB"},
  {lldb_private::MemType::L1, "L1"},
  {lldb_private::MemType::DCACHE, "DCACHE"},
  {lldb_private::MemType::ICACHE, "ICACHE"}
};

static std::unordered_map<DeviceAddressClass, std::vector<GlobalDataType>> AddressClassGlobalDataTypeMap = {
  {DeviceAddressClass::GM, {GlobalDataType::INVALID_TENSOR,
                             GlobalDataType::GENERAL_TENSOR,
                             GlobalDataType::INPUT_TENSOR,
                             GlobalDataType::OUTPUT_TENSOR,
                             GlobalDataType::WORKSPACE_TENSOR,
                             GlobalDataType::MC2_CTX,
                             GlobalDataType::TILING_DATA,
                             GlobalDataType::SHAPE_TENSOR,
                             GlobalDataType::ARGS,
                             GlobalDataType::DEVICE_KERNEL_OBJECT,
                             GlobalDataType::STACK}}
};

static std::unordered_map<DeviceAddressClass, MemType> AddressClassLocalDataTypeMap = {
  {DeviceAddressClass::CBUF, MemType::L1},
  {DeviceAddressClass::CA, MemType::L0A},
  {DeviceAddressClass::CB, MemType::L0B},
  {DeviceAddressClass::CC, MemType::L0C},
  {DeviceAddressClass::UBUF, MemType::UB},
  {DeviceAddressClass::DCACHE, MemType::DCACHE},
  {DeviceAddressClass::ICACHE, MemType::ICACHE}
};

static std::unordered_map<DevdrvChipType, SocType> DevdrvChipSocTypeMap = {
  {DevdrvChipType::CHIP_CLOUD_V2, lldb_private::SocType::ASCEND910B},
  {DevdrvChipType::CHIP_CLOUD_V4, lldb_private::SocType::ASCEND910D}
};

inline std::string CenterText(const std::string& text, size_t width) {
  size_t textLength = text.length();
  if (width < textLength) {
    width = textLength;
  }
  size_t leftPadding = (width - textLength) / 2;
  size_t rightPadding = width - textLength - leftPadding;
  return std::string(leftPadding, ' ') + text + std::string(rightPadding, ' ');
}

inline std::string CenterText(const uint64_t num, size_t width, bool hex, bool valid) {
  std::string text;
  if (hex) {
    std::stringstream ss;
    ss << std::hex << num;
    text = "0x" + ss.str();
  } else {
    text = std::to_string(num);
  }
  if (!valid) {
    text += "(invalid)";
  }
  size_t textLength = text.length();
  if (width < textLength) {
    width = textLength;
  }
  size_t leftPadding = (width - textLength) / 2;
  size_t rightPadding = width - textLength - leftPadding;
  return std::string(leftPadding, ' ') + text + std::string(rightPadding, ' ');
}
} // namespace device_core
} // namespace lldb_private

#endif //LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_ELFCOREDEVICEUTILITIES_H
#endif
