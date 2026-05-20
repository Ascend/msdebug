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

enum class CoreStatus {
  UNKNOWN = 0,
  RUNNING = 1,
  BRKPT = 2,
  SINGLE_STEP = 3,
  EXCEPTION = 4,
  TASK_KILLED = 5
};

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
  SU_INTERRUPT = 0,
  VEC_INTERRUPT_SIMD = 1,
  VEC_INTERRUPT_SIMT = 2,
  UNKNOWN_INTERRUPT_TYPE = 200
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

#pragma pack(4)
struct WarpInfo {
  uint8_t core_id;
  uint8_t warp_id;
  uint8_t warp_num;
  uint64_t simt_pc;
  uint32_t exec_mask;
};
#pragma pack()

struct ThreadInfo {
  uint16_t thread_dim_x;
  uint16_t thread_dim_y;
  uint16_t thread_dim_z;
  uint16_t thread_id;
};

struct InterruptEvent {
  uint8_t core_type;
  InterruptPosType pos_type;
  uint8_t reserve[2];
  CoreStatus status;
  uint32_t core_id;
  uint64_t pc;
  ThreadInfo thread_info;
};

struct ThreadPos {
  uint16_t x;
  uint16_t y;
  uint16_t z;
};

static_assert(sizeof(CoreInfo) <= 64,
    "CoreInfo param size must less than params in DebugSendInfo");

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
  int32_t stream_id;
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
  InterruptPosType pos_type{InterruptPosType::SU_INTERRUPT};
  ThreadPos thread_pos;
};

// Message from lldb-client to lldb-server
struct MemoryTypeInfo {
  uint32_t core_id;
  CoreType core_type;
  DeviceAddressClass address_class;
  uint8_t element_size;
};

//
struct InterruptPosInfo {
  CoreType core_type;
  bool single_core_run;
  bool single_warp_run;
  uint32_t core_id;
  InterruptPosType pos_type{InterruptPosType::SU_INTERRUPT};
  ThreadPos thread_pos;
  uint64_t pc;
  ThreadInfo thread_info;
  uint32_t first_stop_core_id;
  CoreType first_stop_core_type;

  void Update(const InterruptEvent &event) {
    first_stop_core_id = core_id = event.core_id;
    first_stop_core_type = core_type = (CoreType)event.core_type;
    pos_type = event.pos_type;
    pc = event.pc;
    thread_info = event.thread_info;
    if (thread_info.thread_dim_x && thread_info.thread_dim_y &&
        thread_info.thread_dim_z) {
      thread_pos.x = thread_info.thread_id % thread_info.thread_dim_x;
      thread_pos.y = thread_info.thread_id / thread_info.thread_dim_x %
                     thread_info.thread_dim_y;
      thread_pos.z = thread_info.thread_id / thread_info.thread_dim_x /
                     thread_info.thread_dim_y;
    }
  }

  void Reset() {
    core_id = 1;
    core_type = CoreType::UNKNOWN_CORE_TYPE;
    pos_type = InterruptPosType::SU_INTERRUPT;
    thread_pos = ThreadPos{};
    pc = -1;
    // 默认设置所有warp同步调试
    single_warp_run = false;
    thread_info = ThreadInfo{};
  }

  uint16_t GetWarpNum() const {
    return (thread_info.thread_dim_x * thread_info.thread_dim_y * thread_info.thread_dim_z + 31U) / 32U;
  }

  uint16_t GetWarpId() const {
    return thread_info.thread_id / 32U;
  }
};

// bitmap:0|1|2|3
//        aic|aiv|simd_vf|simt_vf
// don't care pos_type
constexpr uint8_t AIC_MASK = 1U << static_cast<int>(CoreType::AIC);
constexpr uint8_t AIV_MASK = 1U << static_cast<int>(CoreType::AIV);
constexpr uint8_t MIX_MASK = AIC_MASK | AIV_MASK;

// only show on SIMD/SIMT
constexpr uint8_t SIMD_MASK_OFFSET = 2;
constexpr uint8_t SIMT_MASK_OFFSET = 3;
constexpr uint8_t SIMD_VF_MASK = 1U << SIMD_MASK_OFFSET;
constexpr uint8_t SIMT_VF_MASK = 1U << SIMT_MASK_OFFSET;
constexpr uint8_t VF_MASK = SIMD_VF_MASK | SIMT_VF_MASK;

inline bool IsRegisterSupport(CoreType core_type, InterruptPosType pos_type,
                              uint8_t mask) {
  // use by coredump when we don't detect error register
  // we maybe just show all registers
  if (pos_type == InterruptPosType::UNKNOWN_INTERRUPT_TYPE) {
    return true;
  }
  // if mask not belong to simd/simt, only consider core_type
  if ((mask & AIC_MASK) || (mask & AIV_MASK)) {
    return mask & (1U << static_cast<int>(core_type));
  }
  if (pos_type == InterruptPosType::VEC_INTERRUPT_SIMD) {
    return mask & SIMD_VF_MASK;
  }
  if (pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
    return mask & SIMT_VF_MASK;
  }
  // su but register is vf
  return false;
}
}

#endif
#endif
