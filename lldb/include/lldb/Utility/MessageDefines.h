/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef MESSAGE_DEFINES_H
#define MESSAGE_DEFINES_H

#include "lldb/lldb-types.h"
#include "lldb/lldb-private-enumerations.h"
#include <set>
#include <vector>

namespace lldb_private {
constexpr uint32_t KERNEL_NAME_SIZE = 2048;

enum class SocType {
  SOC_BEGIN = 0,
  ASCEND910B,
  ASCEND310P,
  ASCEND950,
  SOC_END,
};

// Defined by ts 
enum class CoreType {
  AIC = 0,
  AIV,
  UNKNOWN_CORE_TYPE,
};

enum class MemType : uint32_t {
  L0A = 1,
  L0B = 2,
  L0C = 3,
  UB = 4, /* unified buffer */
  L1 = 5,
  FB = 6,
  OUT_MEM = 7,
  REGISTER = 8,
  SCALAR_BUF = 9,
  DCACHE = 10,
  ICACHE = 11,
  // The compiler marks it as "stack" in the Dwarf debug information, and the current AICore is halted at SIMT VF.
  SIMD_STACK = 12,
  SIMT_STACK = 13,
  MEM_LAST,
};

enum class InterruptPosType : uint8_t {
  STARS_SU_INTERRUPT = 0,
  STARS_VEC_INTERRUPT_SIMD = 1,
  STARS_VEC_INTERRUPT_SIMT = 2
};

struct CoreInfo {
  uint8_t core_id;
  InterruptPosType pos_type;
  uint16_t total_num;
  CoreType core_type; // aic / aiv
  uint32_t stream_id;
  uint32_t task_id;
  uint32_t block_id;
  uint32_t status; // device进程状态信息
  uint64_t pc; // 当前aicore pc位置
  uint16_t thread_dim_x;
  uint16_t thread_dim_y;
  uint16_t thread_dim_z;
  uint16_t reserve1;
};

// Message from runtime stub to lldb-server
struct DeviceInfoMsg {
  int32_t device_id;
  pid_t tgid;
  std::string soc_version;
};

struct KernelInfoMsg {
  std::string kernel_name;
  std::string kernel_hash;
  lldb::addr_t pc_base_addr;
  std::vector<char> elf;
};

// Message from lldb-server to lldb-client
struct DeviceInfo {
  std::vector<uint64_t> aic_bitmaps;
  std::vector<uint64_t> aiv_bitmaps;
  uint32_t device_id;
  std::set<uint32_t> device_ids;
};

struct DeviceBinaryInfo {
  uint64_t pc_base_addr;
  uint32_t reset_all_device_binary;
  std::vector<char> binary;
};

struct KernelInfo {
  char name[KERNEL_NAME_SIZE];
};

// used by lldb-client
struct DeviceStopInfo {
  CoreType core_type;
  uint32_t core_id;
  std::string kernel_name;
  uint64_t base_pc;
  SocType soc_type;
  // used by coredump currently
  std::string stop_description;
};

// Message from lldb-client to lldb-server
struct MemoryTypeInfo {
  uint32_t core_id;
  CoreType core_type;
  DeviceAddressClass address_class;
  uint8_t element_size;
};

}

#endif
#endif
