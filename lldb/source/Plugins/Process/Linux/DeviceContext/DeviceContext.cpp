/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "DeviceContext.h"
#include "Ascend310PDeviceContext.h"
#include "Ascend910BDeviceContext.h"
#include "Ascend950DeviceContext.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/Utility/StreamString.h"
#include "lldb/lldb-enumerations.h"
#include "llvm/Support/Errno.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

using namespace llvm;
using namespace lldb;
using namespace lldb_private;

static int32_t g_timeout = 1000; // ms
static uint32_t g_thread_stack_size = 8 * 1024 * 1024;
constexpr int BLOCK_MEM_SIZE = 32; // B

std::condition_variable g_thread_cv;
std::mutex g_thread_mutex;

enum class CanStatus {
  NEED_WAIT = 0,
  WAITING = 1,
  CAN_RUN = 2
};
CanStatus g_can_status = CanStatus::WAITING;

/* macros for sq/cq driver channel with rts */
#define CMD_MAGIC_WORD 'D'
#define CMD_SQ_SEND _IOR(CMD_MAGIC_WORD, 0, DebugInfo*) // 发送SQ
#define CMD_CQ_RECV _IOWR(CMD_MAGIC_WORD, 1, DebugInfo*) // 读取CQ
#define CMD_GM_COPY _IOR(CMD_MAGIC_WORD, 2, DebugInfo*) // 读写GM
#define CMD_DEV_REGISTER _IOR(CMD_MAGIC_WORD, 3, struct ms_debug_info*) // 注册device id

static std::map<CmdType, std::string> CMD_TO_STRING = {
    {CmdType::ENABLE_DEBUG_MODE, "ENABLE_DEBUG_MODE"},
    {CmdType::DISABLE_DEBUG_MODE, "DISABLE_DEBUG_MODE"},
    {CmdType::SET_HARD_BREAKPOINT, "SET_HARD_BREAKPOINT"},
    {CmdType::UNSET_HARD_BREAKPOINT, "UNSET_HARD_BREAKPOINT"},
    {CmdType::INVALID_INSTR_CACHE, "INVALID_INSTR_CACHE"},
    {CmdType::SINGLE_STEP_DEVICE, "SINGLE_STEP_DEVICE"},
    {CmdType::RESUME_DEVICE, "RESUME_DEVICE"},
    {CmdType::GET_DEVICES_INFO, "GET_DEVICES_INFO"},
    {CmdType::GET_CORES_INFO, "GET_CORES_INFO"},
    {CmdType::READ_REGISTER, "READ_REGISTER"},
    {CmdType::READ_LOCAL_MEMORY, "READ_LOCAL_MEMORY"},
    {CmdType::GET_WARP_INFO, "GET_WARP_INFO"},
    {CmdType::TASK_KILL, "TASK_KILL"}
};

static const std::map<std::string, SocType> SOC_STRING_TO_TYPE = {
    {"Ascend910B1", SocType::ASCEND910B},
    {"Ascend910B2", SocType::ASCEND910B},
    {"Ascend910B3", SocType::ASCEND910B},
    {"Ascend910B4", SocType::ASCEND910B},
    {"Ascend910B2C", SocType::ASCEND910B},
    {"Ascend910B4-1", SocType::ASCEND910B},
    {"Ascend910_9391", SocType::ASCEND910B},
    {"Ascend910_9392", SocType::ASCEND910B},
    {"Ascend910_9381", SocType::ASCEND910B},
    {"Ascend910_9382", SocType::ASCEND910B},
    {"Ascend910_9372", SocType::ASCEND910B},
    {"Ascend910_9362", SocType::ASCEND910B},
    {"Ascend310P1", SocType::ASCEND310P},
    {"Ascend310P2", SocType::ASCEND310P},
    {"Ascend310P3", SocType::ASCEND310P},
    {"Ascend310P3Vir01", SocType::ASCEND310P},
    {"Ascend310P3Vir02", SocType::ASCEND310P},
    {"Ascend310P3Vir03", SocType::ASCEND310P},
    {"Ascend310P3Vir04", SocType::ASCEND310P},
    {"Ascend310P4", SocType::ASCEND310P},
    {"Ascend950DT_950x", SocType::ASCEND950DT},
    {"Ascend950DT_950y", SocType::ASCEND950DT},
    {"Ascend950DT_9571", SocType::ASCEND950DT},
    {"Ascend950DT_9572", SocType::ASCEND950DT},
    {"Ascend950DT_9573", SocType::ASCEND950DT},
    {"Ascend950DT_9574", SocType::ASCEND950DT},
    {"Ascend950DT_9575", SocType::ASCEND950DT},
    {"Ascend950DT_9576", SocType::ASCEND950DT},
    {"Ascend950DT_9577", SocType::ASCEND950DT},
    {"Ascend950DT_9578", SocType::ASCEND950DT},
    {"Ascend950DT_9581", SocType::ASCEND950DT},
    {"Ascend950DT_9582", SocType::ASCEND950DT},
    {"Ascend950DT_9583", SocType::ASCEND950DT},
    {"Ascend950DT_9584", SocType::ASCEND950DT},
    {"Ascend950DT_9585", SocType::ASCEND950DT},
    {"Ascend950DT_9586", SocType::ASCEND950DT},
    {"Ascend950DT_9587", SocType::ASCEND950DT},
    {"Ascend950DT_9588", SocType::ASCEND950DT},
    {"Ascend950DT_9591", SocType::ASCEND950DT},
    {"Ascend950DT_9592", SocType::ASCEND950DT},
    {"Ascend950DT_9595", SocType::ASCEND950DT},
    {"Ascend950DT_9596", SocType::ASCEND950DT},
    {"Ascend950DT_95A1", SocType::ASCEND950DT},
    {"Ascend950DT_95A2", SocType::ASCEND950DT},
    {"Ascend950PR_950z", SocType::ASCEND950},
    {"Ascend950PR_9579", SocType::ASCEND950},
    {"Ascend950PR_957b", SocType::ASCEND950},
    {"Ascend950PR_957c", SocType::ASCEND950},
    {"Ascend950PR_957d", SocType::ASCEND950},
    {"Ascend950PR_9589", SocType::ASCEND950},
    {"Ascend950PR_958b", SocType::ASCEND950},
    {"Ascend950PR_9599", SocType::ASCEND950},
};

std::shared_ptr<DeviceContext> DeviceContext::Factory::GetDeviceContext(
    const std::string &soc_version, ::pid_t pid, int device_id) {
  Log *log = GetLog(LLDBLog::Process);
  const auto &it = SOC_STRING_TO_TYPE.find(soc_version);
  if (it == SOC_STRING_TO_TYPE.end()) {
    LLDB_LOG(log, "unsupported soc version: {0}", soc_version);
    return nullptr;
  }

  const SocType soc_type = it->second;
  switch (soc_type) {
    case SocType::ASCEND910B:
      return std::make_shared<Ascend910BDeviceContext>(pid, device_id);
    case SocType::ASCEND310P:
      return std::make_shared<Ascend310PDeviceContext>(pid, device_id);
    case SocType::ASCEND950:
      return std::make_shared<Ascend950DeviceContext>(pid, device_id);
    case SocType::ASCEND950DT:
      return std::make_shared<Ascend950DTDeviceContext>(pid, device_id);
    default:
      LLDB_LOG(log, "unsupported soc type: {0}", (int)soc_type);
      return nullptr;
  };
}

struct DebugSendInfo {
  CmdType  req_id;
  uint8_t is_return;
  uint8_t reserved[3];
  uint32_t msg_id;
  uint32_t data_len;
  uint8_t params[48];
};


struct DebugInfo {
  uint32_t dev_id;
  int32_t timeout;
  uint32_t data_len;
  int32_t pid;
  uint8_t data[64]; // max sqe size
};

struct LocalMemoryInfo {
  uint8_t core_type;
  uint8_t reserve;
  uint16_t core_id;
  MemType mem_type;
  uint64_t src_addr;
  uint16_t thread_id_x; // simt vf
  uint16_t thread_id_y;
  uint16_t thread_id_z;
  uint16_t reserve1;
};

struct LocalMemoryData {
  uint8_t out[32];
};

struct RegisterParam {
  uint8_t core_type;
  uint8_t reserve[3];
  uint32_t core_id;
  uint64_t src_addr;
};

/* the direction of dma */
enum DmaDirection {
    DEVDRV_DMA_HOST_TO_DEVICE = 0,
    DEVDRV_DMA_DEVICE_TO_HOST = 1
};

struct DmaParam {
  uint64_t host_addr;
  uint64_t device_addr;
  uint64_t size;
  DmaDirection direction;
};

struct TaskKillParam {
  uint32_t stream_id;
  uint32_t reserve;
};

DeviceContext::DeviceContext(const ::pid_t pid, const uint32_t device_id) {
  m_pid = pid;
  m_device_id = device_id;
}

Status DeviceContext::Init()
{
  int32_t fd = open("/dev/drv_debug", O_RDWR);
  if (fd < 0) {
    return Status("open /dev/drv_debug failed with error code: %d", errno);
  }
  DebugInfo debug_info = { m_device_id, g_timeout, 0, m_pid };
  int32_t rtn = ioctl(fd, CMD_DEV_REGISTER, &debug_info);
  LLDB_LOGF(GetLog(LLDBLog::Process), "register rtn=%d", rtn);
  if (rtn != 0) {
    m_init_err.SetErrorStringWithFormat("device %d already be used, init failed.",
                                        m_device_id);
    close(fd);
    return m_init_err;
  }
  m_drv_fd = fd;
  return Status();
}

Status DeviceContext::BaseSqCqComm(CmdType type, const uint8_t *data, const uint8_t len,
                                   uint8_t *out, const uint8_t out_len) const
{
  Status error;
  if (m_drv_fd == -1) {
    error.SetErrorStringWithFormatv("DeviceContext has not be initialized success, reason: {0}", m_init_err);
    return error;
  }
  DebugInfo debug_info = { m_device_id, g_timeout, 0, m_pid };
  if (CMD_TO_STRING.find(type) == CMD_TO_STRING.end()) {
    error.SetErrorStringWithFormat("can't find type %d in CMD_TO_STRING map", int(type));
    return error;
  }
  const char *cmd_name = CMD_TO_STRING[type].c_str();
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "cmd_type={0}({1}), base comm, timeout={2}, data_len={3}", cmd_name, static_cast<uint32_t>(type),
           debug_info.timeout, static_cast<uint32_t>(len));
  DebugSendInfo *send_info = (DebugSendInfo*)debug_info.data;
  send_info->req_id = type;
  send_info->is_return = 1;
  send_info->data_len = len;
  std::fill(send_info->params, send_info->params + sizeof(send_info->params), 0);
  if (data != nullptr && len > 0) {
    if (len > sizeof(send_info->params)) {
      error.SetErrorStringWithFormat("payload too large, len=%u>params=%lu", len, sizeof(send_info->params));
      return error;
    }
    std::copy(data, data + len, send_info->params);
  }
  int32_t rtn = ioctl(m_drv_fd, CMD_SQ_SEND, &debug_info);
  if (rtn != 0) {
    error.SetErrorStringWithFormat("call sq %s failed: %d", cmd_name, rtn);
    return error;
  }
  rtn = ioctl(m_drv_fd, CMD_CQ_RECV, &debug_info);
  if (rtn != 0) {
    error.SetErrorStringWithFormat("call cq %s failed: %d", cmd_name, rtn);
    return error;
  }
  DebugRecvInfo *recv_info = (DebugRecvInfo*)debug_info.data;
  if (out != nullptr) {
    size_t size = std::min(size_t(out_len), sizeof(recv_info->recv_msg));
    for (size_t i = 0; i < size; ++i) {
      *(out + i) = recv_info->recv_msg[i];
    }
  }
  if (recv_info->return_val != 0) {
    error.SetErrorStringWithFormat("call ts %s failed: %d", cmd_name, recv_info->return_val);
    return error;
  }
  return error;
}

Status DeviceContext::EnableDebugMode() {
  return BaseSqCqComm(CmdType::ENABLE_DEBUG_MODE);
}

bool DeviceContext::StartListenThread() {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOGF(log, "Enter start listen thread");
  llvm::Expected<HostThread> private_state_thread =
    ThreadLauncher::LaunchThread(
        "listen_receive",
        [this] {
          return RunListenThread();
        }, g_thread_stack_size);

  if (!private_state_thread) {
    LLDB_LOG(GetLog(LLDBLog::Host), "failed to launch host thread: {}",
        llvm::toString(private_state_thread.takeError()));
    return false;
  }

  assert(private_state_thread->IsJoinable());
  m_listen_thread = *private_state_thread;
  return true;
}

void DeviceContext::SetCanRun(bool can) {
  Log *log = GetLog(LLDBLog::Process);
  if (can) {
    std::unique_lock<std::mutex> lk(g_thread_mutex);
    g_can_status = CanStatus::CAN_RUN;
    g_thread_cv.notify_all();
    LLDB_LOGF(log, "SetCanRun true success");
  } else {
    std::unique_lock<std::mutex> lk(g_thread_mutex);
    if (g_can_status == CanStatus::WAITING) {
      LLDB_LOGF(log, "SetCanRun is already waiting");
      return;
    }
    g_can_status = CanStatus::NEED_WAIT;
    static constexpr uint32_t WAIT_SEC = 2;
    if (g_thread_cv.wait_for(lk, std::chrono::seconds(WAIT_SEC),
                             [](){return g_can_status == CanStatus::WAITING;})) {
      LLDB_LOGF(log, "SetCanRun false success");
    } else {
      LLDB_LOGF(log, "SetCanRun false failed, timeout");
    }
  }
}

thread_result_t DeviceContext::RunListenThread() {
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  m_close = false;

  DebugInfo debug_info = { m_device_id, g_timeout, 0, m_pid };
  DebugRecvInfo *recv_info = (DebugRecvInfo*)debug_info.data;
  while (!m_close) {
    if (g_can_status != CanStatus::CAN_RUN) {
      std::unique_lock<std::mutex> lk(g_thread_mutex);
      g_can_status = CanStatus::WAITING;
      g_thread_cv.notify_all();
      g_thread_cv.wait(lk, [](){return g_can_status == CanStatus::CAN_RUN;});
    }
    LLDB_LOGF(log, "start detect ts event");
    int32_t rtn = ioctl(m_drv_fd, CMD_CQ_RECV, &debug_info);
    if (rtn != 0) {
      continue;
    }
    if (recv_info->cmd_type == CmdType::INTERRUPT_EVENT) {
      LLDB_LOGF(log, "receive interrupt event");
      if (m_bp_callback) {
        m_bp_callback(*recv_info);
      }
    } else {
      LLDB_LOGF(log, "expect no other cmd_type message here: %d", int(recv_info->cmd_type));
      continue;
    }

    LLDB_LOGF(log, "wait for resume then detect");
    std::unique_lock<std::mutex> lk(g_thread_mutex);
    g_can_status = CanStatus::WAITING;
  }
  LLDB_LOGF(log, "exit run listen thread");
  return {};
}

inline bool IsValidLocalAddr(lldb::addr_t addr, size_t size) {
  constexpr uint64_t MAX_LOCAL_MEM_SIZE = 1ULL << 30; // 1GB
  if (size == 0 || size >= MAX_LOCAL_MEM_SIZE || addr + size >= MAX_LOCAL_MEM_SIZE) {
    return false;
  }
  return true;
}

bool DeviceContext::IsValidStack(addr_t addr, size_t size) {
  constexpr uint64_t START_STACK_VA = 0x10000;
  constexpr uint64_t END_STACK_VA = 0x1000000;
  constexpr uint64_t MAX_STACK_SIZE = END_STACK_VA - START_STACK_VA;

  if (size == 0 || size >= MAX_STACK_SIZE) {
    return false;
  }
  if (addr < START_STACK_VA || addr >= END_STACK_VA || addr + size >= END_STACK_VA) {
    return false;
  }
  return true;
}

inline bool DeviceContext::IsValidGlobalAddr(lldb::addr_t addr, size_t size) {
  constexpr uint64_t START_DEVICE_ADDR = 0x020000000000;
  constexpr uint64_t END_DEVICE_ADDR = 0x330000000000;
  constexpr uint64_t MAX_GLOBAL_SIZE = 1ULL << 48;

  if (size == 0 || size >= MAX_GLOBAL_SIZE) {
    return false;
  }
  if (addr < START_DEVICE_ADDR || addr >= END_DEVICE_ADDR ||
      addr + size >= END_DEVICE_ADDR) {
    return false;
  }
  return true;
}

size_t DeviceContext::ReadLocalMemory(lldb::addr_t addr, size_t size,
                                      const MemoryTypeInfo &memory_type_info,
                                      const InterruptPosInfo &pos_info,
                                      void *data) {
  Log *log = GetLog(LLDBLog::Process);
  LocalMemoryInfo event{};
  event.core_id = pos_info.core_id;
  event.mem_type = DeviceAddressClassToMemType(memory_type_info.address_class);
  if (memory_type_info.address_class == DeviceAddressClass::STACK) {
    if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
      event.mem_type = MemType::SIMT_STACK;
    } else if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMD) {
      event.mem_type = MemType::SIMD_STACK;
    }
  }
  uint32_t align_byte = addr % BLOCK_MEM_SIZE;
  event.src_addr = addr - align_byte;
  event.core_type = static_cast<uint8_t>(pos_info.core_type);
  event.thread_id_x = pos_info.thread_pos.x;
  event.thread_id_y = pos_info.thread_pos.y;
  event.thread_id_z = pos_info.thread_pos.z;
  LLDB_LOG(log,
           "Read local memory: {0:x}, core_id={1}, core_type={2}, mem_type={3}",
           addr, event.core_id, event.core_type,
           static_cast<uint32_t>(event.mem_type));

  LocalMemoryData mem_data;
  if (INT32_MAX - align_byte < size) {
    return 0;
  }
  size_t left_size = size + align_byte;
  uint8_t *cur_data = (uint8_t *)data;
  while (left_size > 0) {
    Status error = BaseSqCqComm(CmdType::READ_LOCAL_MEMORY, (const uint8_t *)&event, sizeof(event),
                                (uint8_t *)&mem_data, sizeof(mem_data));
    if (error.Fail()) {
      LLDB_LOG(log, "Read local memory failed: {0}", error);
      return 0;
    }
    // check size
    if (left_size > INT32_MAX) {
      LLDB_LOG(log, "Read local memory byte size error.");
      return 0;
    }
    size_t real_size = std::min((size_t)BLOCK_MEM_SIZE, left_size) - align_byte;
    for (size_t i = 0; i < real_size; ++i) {
      cur_data[i] = mem_data.out[align_byte + i];
    }
    LLDB_LOG(log, "cpy success, left_size: {0}", left_size);
    cur_data += real_size;
    align_byte = 0;
    left_size = left_size < BLOCK_MEM_SIZE ? 0: left_size - BLOCK_MEM_SIZE;
    event.src_addr += BLOCK_MEM_SIZE;
  }
  if (log) {
    StreamString ss;
    ss << "0x";
    cur_data = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < std::min(size, 8UL); i++) {
      ss.Printf("%02x", cur_data[i]);
    }
    LLDB_LOG(log, "Read local memory, first {0} byte value: {1}", std::min(size, 8UL), ss.GetString());
  }
  return size;
}

inline CoreMaskInfo GenCoreMask(const InterruptPosInfo &pos_info) {
    CoreMaskInfo core_info{};
    if (!pos_info.single_core_run) {
        return core_info;
    }
    core_info.magic = 0x5a5a5a5a;
    // A2/A3/310P: uint32_t aic_mask, uint64_t aiv_mask
    if (pos_info.core_type == CoreType::AIC) {
        core_info.aic_mask = 1U << pos_info.core_id;
    } else {
        core_info.aiv_mask = 1ULL << pos_info.core_id;
    }
    return core_info;
}

Status DeviceContext::Resume(const InterruptPosInfo &pos_info) const {
  CoreMaskInfo maskInfo = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::RESUME_DEVICE, (uint8_t*)&maskInfo, sizeof(maskInfo));
}

Status DeviceContext::SingleStep(const InterruptPosInfo &pos_info) const {
  CoreMaskInfo maskInfo = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::SINGLE_STEP_DEVICE, (uint8_t*)&maskInfo, sizeof(maskInfo));
}

Status DeviceContext::ReadRegister(const RegisterInfo *reg_info, const InterruptPosInfo &pos_info, RegisterValue &value) const {
  if (!m_reg_info_up) {
    return Status("internal error: need initialize m_reginster_info");
  }
  return m_reg_info_up->ReadRegister(reg_info, pos_info, this, value);
}


Status DeviceContext::ReadRegister(uint64_t addr, const RegisterInfo *reg_info,
                                   uint32_t core_id, CoreType core_type,
                                   RegisterValue &reg_value) const {
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  constexpr uint8_t max_bytes = 32;
  WritableDataBufferSP buffer_sp(new DataBufferHeap(max_bytes, 0));
  RegisterParam param;
  param.core_id = core_id;
  param.core_type = static_cast<uint8_t>(core_type);
  param.src_addr = addr;
  error = CheckRegisterAddr(core_type, param.src_addr);
  if (error.Fail()) {
    return error;
  }
  error = BaseSqCqComm(CmdType::READ_REGISTER, (const uint8_t *)&param, sizeof(param),
                       (uint8_t *)buffer_sp->GetBytes(), buffer_sp->GetByteSize());
  LLDB_LOG(log, "Read {0} Register core_id={1} addr={2:x} core_type={3}",
           reg_info->name, core_id, addr, int(core_type));
  if (error.Fail()) {
    return error;
  }

  reg_value.SetFromMemoryData(*reg_info, buffer_sp->GetBytes(),
                              reg_info->byte_size, eByteOrderLittle, error);
  if (log) {
    StreamString ss;
    ss << "0x";
    uint8_t *cur_data = buffer_sp->GetBytes();
    for (size_t i = 0; i < std::min(reg_info->byte_size, 8U); i++) {
      ss.Printf("%02x", cur_data[i]);
    }
    LLDB_LOG(log, "Read register {0}, first {1} byte value: {2}",
             reg_info->name,
             std::min(reg_info->byte_size, 8U), ss.GetString());
  }
  return error;
}

Status DeviceContext::GetDeviceInfo(DeviceInfo &device_info) {
  Status error;
  TsDeviceInfo info;
  Log *log = GetLog(LLDBLog::Process);
  error = BaseSqCqComm(CmdType::GET_DEVICES_INFO, nullptr, 0,
                       (uint8_t *)&info, sizeof(info));
  if (error.Fail()) {
    return error;
  }
  device_info.aic_bitmaps.clear();
  device_info.aiv_bitmaps.clear();
  device_info.aiv_bitmaps.push_back(info.aiv_bitmap);
  device_info.aic_bitmaps.push_back(info.aic_bitmap);
  device_info.device_id = m_device_id;
  LLDB_LOGF(log, "GetDeviceInfo device_id=%d aic_bitmap=%#lx aiv_bitmap=%#lx",
            device_info.device_id, info.aic_bitmap, info.aiv_bitmap);
  return error;
}

Status DeviceContext::GetCoresInfo(std::vector<CoreInfo> &cores_info) {
  Status error;
  Log *log = GetLog(LLDBLog::Process);
  cores_info.clear();
  CoreInfoParam param{};
  param.info_idx = 0;
  CoreInfo core_info;
  error = BaseSqCqComm(CmdType::GET_CORES_INFO, (uint8_t*)&param, sizeof(CoreInfoParam),
                       (uint8_t *)&core_info, sizeof(CoreInfo));
  if (error.Fail()) {
    return error;
  }
  LLDB_LOGF(log, "GetCoresInfo total_num=%u", core_info.total_num);
  cores_info.push_back(core_info);

  for (int i = 1; i < core_info.total_num; i++) {
    param.info_idx = i;
    error = BaseSqCqComm(CmdType::GET_CORES_INFO, (uint8_t*)&param, sizeof(CoreInfoParam),
                         (uint8_t *)&core_info, sizeof(CoreInfo));
    if (error.Fail()) {
        error.SetErrorStringWithFormatv("get {0} core failed: {1}", i, error);
        return error;
    }
    cores_info.push_back(core_info);
    LLDB_LOGF(log, "GetCoresInfo core_id=%d core_type=%d total_num=%u",
              core_info.core_id, int(core_info.core_type), core_info.total_num);
  }

  m_cores_info = cores_info;
  return error;
}

void DeviceContext::SetBreakpointCallback(const Callback &callback) {
  m_bp_callback = callback;
}

bool DeviceContext::UseSpecifiedCore(uint32_t aic_mask, uint64_t aiv_mask, CoreMaskInfo &maskInfo)
{
  if (aic_mask == 0 && aiv_mask == 0) {
    return false;
  }
  maskInfo.magic = 0x5a5a5a5a; // The mask information is read from the TS.
  maskInfo.aic_mask = aic_mask;
  maskInfo.aiv_mask = aiv_mask;
  return true;
}

Status DeviceContext::InvalidInstrCache(const addr_t &addr,
  const InterruptPosInfo &pos_info, uint8_t redirect_ifu) const {
  Status error;
  InvalidCacheParam param;
  param.enable_all = 0;
  param.virt_addr = addr;
  param.redirect_ifu = redirect_ifu;
  param.core_info = GenCoreMask(pos_info);
  return BaseSqCqComm(CmdType::INVALID_INSTR_CACHE, (uint8_t*)&param, sizeof(param));
}

size_t DeviceContext::ReadGlobalMemory(addr_t addr, size_t size, void *data) {
  Log *log = GetLog(LLDBLog::Process);
  if (size == 0 || m_drv_fd == -1) {
    LLDB_LOG(log, "DeviceContext has not be initialized success or size=0, reason {0}", m_init_err);
    return 0;
  }
  if (!IsValidGlobalAddr(addr, size)) {
    LLDB_LOG(log, "Invalid global addr={0}, size={1}", addr, size);
    return 0;
  }

  // GM can be read via driver api but only one page for once.
  // So it is necessary to check if the host address "param->host_addr" crosses
  // pages. If so, calls to the driver interface should be made in batches, with
  // each page being called separately.

  constexpr uint64_t page_size = 4096;
  uint8_t *dest = static_cast<uint8_t *>(data);
  std::vector<uint8_t> tmp_data(size, 0);

  // calculate page numbers
  uint64_t host_addr = reinterpret_cast<uint64_t>(&tmp_data[0]);
  uint64_t addr_len = static_cast<uint64_t>(size);
  uint64_t align_addr_len = ((host_addr & (page_size - 1)) + addr_len);
  uint64_t total_page_num = align_addr_len / page_size;
  if ((align_addr_len & (page_size - 1)) != 0) {
    total_page_num++;
  }

  LLDB_LOG(log, "Total pages to read: {0}", total_page_num);

  uint64_t bytes_read_total = 0;
  uint64_t current_device_addr = addr;

  // call driver api once for each page
  for (uint64_t i = 0; i < total_page_num; i++) {
    // calculate byte size for current page
    uint64_t current_host_addr = host_addr + bytes_read_total;
    uint64_t offset_in_page = current_host_addr % page_size;
    uint64_t bytes_this_page = page_size - offset_in_page;
    uint64_t bytes_to_read = std::min(bytes_this_page, size - bytes_read_total);

    // prepare parameters for driver api
    DebugInfo debug_info = {m_device_id, g_timeout, 0, m_pid};
    DmaParam *param = (DmaParam *)debug_info.data;
    param->host_addr = current_host_addr;
    param->device_addr = current_device_addr;
    param->size = bytes_to_read;
    param->direction = DEVDRV_DMA_DEVICE_TO_HOST;

    LLDB_LOGF(
        log, "Reading page %lu/%lu: host_addr=%#lx, device_addr=%#lx, size=%lu",
        i, total_page_num, param->host_addr, param->device_addr, param->size);

    // call driver api
    int32_t rtn = ioctl(m_drv_fd, CMD_GM_COPY, &debug_info);
    if (rtn != 0) {
      LLDB_LOGF(log, "call gm copy failed: %d", rtn);
      return bytes_read_total;
    }

    bytes_read_total += bytes_to_read;
    current_device_addr += bytes_to_read;
  }

  // 最后把tmpData中的所有数据复制到目标位置
  std::copy(tmp_data.begin(), tmp_data.end(), dest);
  return bytes_read_total;
}

size_t DeviceContext::WriteGlobalMemory(addr_t addr, size_t size, const void *data) {
  Log *log = GetLog(LLDBLog::Process);
  if (size == 0 || m_drv_fd == -1) {
    LLDB_LOG(log, "DeviceContext has not be initialized success or size=0, reason {0}", m_init_err);
  }
  if (!IsValidGlobalAddr(addr, size)) {
    LLDB_LOG(log, "Invalid global addr={0}, size={1}", addr, size);
    return 0;
  }
  DebugInfo debug_info = { m_device_id, g_timeout, 0, m_pid };
  DmaParam *param = (DmaParam*)debug_info.data;
  uint8_t *data_ptr = static_cast<uint8_t *>(const_cast<void *>(data));
  std::vector<uint8_t> tmpData(data_ptr, data_ptr + size);
  param->host_addr = (uint64_t)&tmpData[0];
  param->device_addr = addr;
  param->size = size;
  param->direction = DEVDRV_DMA_HOST_TO_DEVICE;
  LLDB_LOGF(log, "host_addr=%#lx, device_addr=%#lx, size=%lu, direction=%d",
            param->host_addr, param->device_addr, param->size, param->direction);
  int32_t rtn = ioctl(m_drv_fd, CMD_GM_COPY, &debug_info);
  if (rtn != 0) {
      LLDB_LOGF(log, "call gm copy failed: %d", rtn);
      return 0;
  }
  return size;
}

size_t DeviceContext::ReadMemory(lldb::addr_t addr, size_t size, const MemoryTypeInfo &memory_type_info,
                                 const InterruptPosInfo &pos_info, void *out) {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOGF(log, "ReadMemory device_addr=%#lx, size=%lu", addr, size);
  auto address_class = memory_type_info.address_class;
  auto mem_type = DeviceAddressClassToMemType(address_class);
  if (address_class == DeviceAddressClass::STACK) {
    if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
      mem_type = MemType::SIMT_STACK;
    } else if (pos_info.pos_type == InterruptPosType::VEC_INTERRUPT_SIMD) {
      mem_type = MemType::SIMD_STACK;
    }
  }
  LLDB_LOG(log, "{0} query address_class={1}, mem_type={2}", __FUNCTION__,
      static_cast<uint32_t>(address_class), static_cast<uint32_t>(mem_type));
  if (mem_type == MemType::MEM_LAST) {
    LLDB_LOG(log, "Memory type is unknown, read memory from ascend device failed, address_class is {0}",
             (int)address_class);
    return 0;
  }
  // check
  if (address_class == DeviceAddressClass::GM) {
    if (!IsValidGlobalAddr(addr, size)) {
      LLDB_LOG(log, "Invalid global addr or size.");
      return 0;
    }
  } else if (address_class == DeviceAddressClass::STACK && mem_type == MemType::OUT_MEM) {
    if (!(IsValidStack(addr, size) || IsValidGlobalAddr(addr, size))) {
      LLDB_LOG(log, "Invalid stack addr or size.");
      return 0;
    }
  } else if (!IsValidLocalAddr(addr, size)) {
    LLDB_LOG(log, "Invalid local addr or size.");
    return 0;
  }
  // can read gm and local memory
  return ReadLocalMemory(addr, size, memory_type_info, pos_info, out);
}

Status DeviceContext::SetSoftwareBreakpoint(lldb::addr_t addr) {
  return Status();
}

Status DeviceContext::RemoveSoftwareBreakpoint(lldb::addr_t addr) {
  return Status();
}

Status DeviceContext::TaskKill(uint32_t stream_id) {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "DeviceContext::{0}", __FUNCTION__);
  TaskKillParam param;
  param.stream_id = stream_id;
  SetCanRun(false);
  Status error = BaseSqCqComm(CmdType::TASK_KILL, (uint8_t*)&param, sizeof(param));
  SetCanRun(true);
  return error;
}

MemType DeviceContext::DeviceAddressClassToMemType(DeviceAddressClass address_class) const {
  if (address_class == DeviceAddressClass::STACK) {
    // stack is located in different memory space in different chips
    return GetStackMemType();
  }
  static std::map<DeviceAddressClass, MemType> address_class_mem_type_map = {
    {DeviceAddressClass::GM, MemType::OUT_MEM},
    {DeviceAddressClass::CBUF, MemType::L1},
    {DeviceAddressClass::CA, MemType::L0A},
    {DeviceAddressClass::CB, MemType::L0B},
    {DeviceAddressClass::CC, MemType::L0C},
    {DeviceAddressClass::UBUF, MemType::UB},
    {DeviceAddressClass::FBUF, MemType::FB},
  };
  auto map_entry = address_class_mem_type_map.find(address_class);
  if (map_entry == address_class_mem_type_map.end()) {
    return MemType::MEM_LAST;
  }
  return map_entry->second;
}

void DeviceContext::SetSocket(const Socket *client_socket) {
  m_client_socket = client_socket;
}

bool DeviceContext::IsDeviceIdMatched(const uint32_t device_id) const {
  return m_device_id == device_id;
}

bool DeviceContext::IsTgidMatched(const int32_t tgid) const {
  return m_pid == tgid;
}

void DeviceContext::UpdateTgid(const int32_t tgid) {
  m_pid = tgid;
}

DeviceContext::~DeviceContext() {
  if (m_listen_thread.IsJoinable()) {
    thread_result_t result = {};
    m_close = true;
    g_can_status = CanStatus::CAN_RUN;
    g_thread_cv.notify_all();
    m_listen_thread.Join(&result);
    m_listen_thread.Reset();
  }
  if (m_drv_fd != -1) {
    close(m_drv_fd);
  }
}
#endif // ifdef MS_DEBUGGER
