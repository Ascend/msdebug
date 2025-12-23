/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "CommandObjectAscend.h"

#include <memory>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "lldb/Host/OptionParser.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#include "lldb/Target/Process.h"
#include "lldb/Utility/Status.h"
#include "lldb/Utility/AscendVerification.h"
#include "lldb/Core/Debugger.h"
#include "Plugins/Process/Linux/DeviceContext/DeviceContext.h"
#include "lldb/Target/Thread.h"
#include "lldb/Interpreter/Options.h"
#include "lldb/Target/StopInfo.h"
#include "Plugins/Process/elf-core/device-core/ElfCoreDeviceUtilities.h"

using namespace lldb;
using namespace lldb_private;
using namespace device_core;
using namespace std;

static constexpr uint32_t DEVICE_ID_WIDTH = 4;
static bool CheckStringValid(const std::string &arg, CommandReturnObject &result) {
  std::string msg = "";
  if (CheckStringValid(arg, msg)) {
    result.AppendErrorWithFormat("Invalid character '%s'", msg.c_str());
    return false;
  }
  return true;
}

inline uint32_t CalcBitNum(const vector<uint64_t> &bitmaps) {
  uint32_t num = 0;
  for (auto bitmap: bitmaps) {
    num += __builtin_popcountll(bitmap);
  }
  return num;
}
 
inline void DumpBitmap(const vector<uint64_t> &bitmaps, std::ostream &os) {
  os << "0x";
  if (bitmaps.empty()) {
    os << 0;
    return;
  }
  // 高位在前
  for (int i = bitmaps.size() - 1; i >= 0; i --) {
    os << std::hex << bitmaps[i];
    if (i > 0) {
      os << " ";
    }
  }
}

// CommandObjectAscendInfoDevices
class CommandObjectAscendInfoDevices : public CommandObjectParsed {
public:
  explicit CommandObjectAscendInfoDevices(CommandInterpreter &interpreter)
      : CommandObjectParsed(
            interpreter, "ascend info devices",
            "show devices overall info.  "
            "",
            "ascend info devices",
            eCommandRequiresProcess | eCommandTryTargetAPILock |
                eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                eCommandProcessMustBePausedInDevice | eCommandProcessMustNotBeTaskKilled) {}

  ~CommandObjectAscendInfoDevices() override = default;

protected:
  void WriteLine(std::ostream& os, uint32_t id, bool is_current, uint32_t aic_num,
                  uint32_t aiv_num, const vector<uint64_t> &aic_bitmaps,
                  const vector<uint64_t> &aiv_bitmaps) {
    os << (is_current ? "*    " : "     ")
      << std::dec << id << "      "
      << aic_num << "       "
      << aiv_num << "      ";
    DumpBitmap(aic_bitmaps, os);
    os << "     ";
    DumpBitmap(aiv_bitmaps, os);
    os << '\n';
  }

  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }

    // 查询当前使用的device
    DeviceInfo device_info;
    Status error = process->GetDeviceInfo(device_info);
    if (error.Success()) {
      uint32_t aic_num = CalcBitNum(device_info.aic_bitmaps);
      uint32_t aiv_num = CalcBitNum(device_info.aiv_bitmaps);
      strm << "  Device Aic_Num Aiv_Num Aic_Mask Aiv_Mask\n";
      std::stringstream ss;
      for (const auto& id : device_info.device_ids) {
        WriteLine(ss, id, id == device_info.device_id, aic_num, aiv_num, 
          device_info.aic_bitmaps, device_info.aiv_bitmaps);
      }
      strm << ss.str();
    } else {
      strm << "[Failed to get ascend info, reason:" << error.AsCString() << "]";
    }
    strm.EOL();
    strm.Flush();
  }
};

class AscendCommonInfo {
public:
  std::vector<CoreInfo> m_core_info_vec;
  DeviceInfo m_device_info;

  // 获取当前聚焦的core id
  uint8_t m_focused_info_index;
  DeviceStopInfo m_stop_info;

  Status GetCommonInfo(Process *process) {
    Status error = process->GetDeviceInfo(m_device_info);
    if (!error.Success()) {
      return error;
    }
    error = process->GetCoresInfo(m_core_info_vec);
    if (!error.Success()) {
      return error;
    }

    process->GetDeviceStopInfoCached(m_stop_info);
    for (uint32_t i = 0; i < m_core_info_vec.size(); i++) {
      if ((m_core_info_vec[i].core_id == m_stop_info.core_id) &&
        (m_core_info_vec[i].core_type == m_stop_info.core_type)) {
        m_focused_info_index = i;
      }
    }

    return error;
  }

  Status GetFocusCore(Process *process, CoreInfo &core_info) {
    Status error = GetCommonInfo(process);
    if (!error.Success()) {
      return error;
    }

    core_info = m_core_info_vec[m_focused_info_index];
    return error;
  }
};

// show state changed info and stopped point context
void ShowInfoWhenStopped(Process &process, Thread &thread, Stream &strm) {
  process.ShowDeviceStopInfoCached(strm);
  thread.RefreshStackFramePC();
  if (process.GetTarget().GetExecutableModule()) {
    thread.GetStatus(strm, 0, 1, 1, true);
  }
}

// CommandObjectAscendInfoAicores

class CommandObjectAscendInfoAicores : public CommandObjectParsed {
public:
  explicit CommandObjectAscendInfoAicores(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend info cores",
                            "show aicores overall info.  "
                            "",
                            "ascend info cores",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDevice) {}

  ~CommandObjectAscendInfoAicores() override = default;

protected:
  
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }
    AscendCommonInfo commonInfo;
    Status error = commonInfo.GetCommonInfo(process);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get ascend info, reason: %s", error.AsCString());
      return;
    }

    auto stop_reason = process->GetCoreStopReason();
    strm << "  CoreId  Type  Device Stream Task Block         PC               stop reason\n";
    for (uint32_t i = 0; i < commonInfo.m_core_info_vec.size(); ++i) {
      std::stringstream ss;
      std::string core_type_str;
      auto &core_info = commonInfo.m_core_info_vec[i];
      if (core_info.core_type == CoreType::AIC) {
        core_type_str = "aic";
      } else {
        core_type_str = "aiv";
      }
      if (commonInfo.m_focused_info_index == i) {
        ss << "*";
      } else {
        ss << " ";
      }
      static constexpr uint32_t CORE_ID_WIDTH = 4;
      static constexpr uint32_t BLOCK_ID_WIDTH = 6;
      
      std::string stopReasonKey = std::to_string(static_cast<uint8_t>(core_info.core_type)) + '_' +
        std::to_string(core_info.core_id);
      auto iter = stop_reason.find(stopReasonKey);
      ss << std::setw(CORE_ID_WIDTH) << static_cast<uint32_t>(core_info.core_id) << "     " <<
        core_type_str << "      " << commonInfo.m_device_info.device_id << "     " <<
        core_info.stream_id << "     " << core_info.task_id << std::setw(BLOCK_ID_WIDTH) <<
        core_info.block_id << "     0x" << std::hex << core_info.pc <<
        "         " << ((iter == stop_reason.end()) ? "NULL" : iter->second) << std::endl;
      strm << ss.str();
    }

    strm.EOL();
    strm.Flush();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
  }
};

// CommandObjectAscendInfoTasks

class CommandObjectAscendInfoTasks : public CommandObjectParsed {
public:
  explicit CommandObjectAscendInfoTasks(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend info tasks",
                            "show tasks overall info.  "
                            "",
                            "ascend info tasks",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDevice | eCommandProcessMustNotBeTaskKilled) {}

  ~CommandObjectAscendInfoTasks() override = default;

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }
    AscendCommonInfo commonInfo;
    Status error = commonInfo.GetCommonInfo(process);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get ascend info, reason: %s", error.AsCString());
      return;
    }

    KernelInfo kernel_info;
    error = process->GetKernelInfo(kernel_info);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get kernel info, reason: %s", error.AsCString());
      return;
    }

    CoreInfo focus_core;
    error = commonInfo.GetFocusCore(process, focus_core);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get focused core info, reason: %s", error.AsCString());
      return;
    }

    TaskInfo task_info;

    task_info.stream_id = focus_core.stream_id;
    task_info.task_id = focus_core.task_id;
    for (size_t i = 0; i < sizeof(task_info.invocation) - 1; ++i) {
      task_info.invocation[i] = kernel_info.name[i];
    }

    static constexpr uint32_t STREAM_ID_WIDTH = 8;
    static constexpr uint32_t TASK_ID_WIDTH = 6;
    strm << "  Device Stream Task Invocation\n";
    std::stringstream ss;
    ss << "*";
    ss << std::setw(DEVICE_ID_WIDTH) << commonInfo.m_device_info.device_id
       << std::setw(STREAM_ID_WIDTH) << task_info.stream_id
       << std::setw(TASK_ID_WIDTH) << task_info.task_id <<"  "
       << task_info.invocation <<std::endl;
    strm << ss.str();
    strm.EOL();
    strm.Flush();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
  }
};

// CommandObjectAscendInfoStream

class CommandObjectAscendInfoStream : public CommandObjectParsed {
public:
  explicit CommandObjectAscendInfoStream(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend info stream",
                            "show stream overall info.  "
                            "",
                            "ascend info stream",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDevice | eCommandProcessMustNotBeTaskKilled) {}

  ~CommandObjectAscendInfoStream() override = default;

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }
    AscendCommonInfo commonInfo;
    Status error = commonInfo.GetCommonInfo(process);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get ascend info, reason: %s", error.AsCString());
      return;
    }
    CoreInfo focus_core;
    error = commonInfo.GetFocusCore(process, focus_core);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get focused core info, reason: %s", error.AsCString());
      return;
    }
    
    StreamInfo stream_info;
    stream_info.stream_id = focus_core.stream_id;
    std::string core_type_str;
    if (focus_core.core_type == CoreType::AIC) {
      core_type_str = "aic";
    } else {
      core_type_str = "aiv";
    }

    strm << "  Device Stream Type\n";
    std::stringstream ss;
    ss << "*";
    ss << std::setw(DEVICE_ID_WIDTH) <<commonInfo.m_device_info.device_id  << "      "
       << stream_info.stream_id << "    "
       << core_type_str << std::endl;
    strm << ss.str();

    strm.EOL();
    strm.Flush();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
  }
};

// CommandObjectAscendInfoBlocks

#define LLDB_OPTIONS_ascend_info_blocks
#include "CommandOptions.inc"

class CommandObjectAscendInfoBlocks : public CommandObjectParsed {
public:

  class OptionGroupAscendInfoBlocks : public OptionGroup {
  public:
    OptionGroupAscendInfoBlocks() {}
 
    ~OptionGroupAscendInfoBlocks() override = default;
 
    llvm::ArrayRef<OptionDefinition> GetDefinitions() override {
      return llvm::ArrayRef(g_ascend_info_blocks_options);
    }
 
    Status SetOptionValue(uint32_t option_idx, llvm::StringRef option_value,
                          ExecutionContext *execution_context) override {
      Status error;
      const int short_option = g_ascend_info_blocks_options[option_idx].short_option;
 
      switch (short_option) {
        case 'd':
          m_show_details = true;
          break;
 
        default:
          llvm_unreachable("Unimplemented option");
      }
      return error;
    }
    
    void OptionParsingStarting(ExecutionContext *execution_context) override {
      m_show_details = false;
    }
 
    bool m_show_details;
  };

  explicit CommandObjectAscendInfoBlocks(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend info blocks",
                            "show blocks overall info.  "
                            "",
                            "ascend info blocks",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDevice | eCommandProcessMustNotBeTaskKilled) {
    m_all_options.Append(&m_blocks_options);
    m_all_options.Finalize();
  }

  ~CommandObjectAscendInfoBlocks() override = default;

  Options *GetOptions() override { return &m_all_options; }

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }
    AscendCommonInfo commonInfo;
    Status error = commonInfo.GetCommonInfo(process);
    if (!error.Success()) {
      result.AppendErrorWithFormat("Failed to get ascend info, reason: %s", error.AsCString());
      return;
    }

    if (m_blocks_options.m_show_details) {
      ShowBlockInfoDetails(command, result, commonInfo);
      return;
    }

    std::vector<BlockInfo> block_info_vec(commonInfo.m_core_info_vec.size());

    for (uint32_t i = 0; i < commonInfo.m_core_info_vec.size(); i++) {
      block_info_vec[i].stream_id = commonInfo.m_core_info_vec[i].stream_id;
      block_info_vec[i].task_id = commonInfo.m_core_info_vec[i].task_id;
      block_info_vec[i].block_id = commonInfo.m_core_info_vec[i].block_id;
    }

    strm << "  Device Stream Task Block\n";
    for (uint32_t i = 0; i < block_info_vec.size(); ++i) {
      std::stringstream ss;
      if (commonInfo.m_focused_info_index != i) {
          ss << " ";
      }else {
          ss << "*";
      }
      ss << std::setw(DEVICE_ID_WIDTH) <<commonInfo.m_device_info.device_id << "      "
         << block_info_vec[i].stream_id << "     "
         << block_info_vec[i].task_id << "     "
         << block_info_vec[i].block_id << std::endl;
      strm << ss.str();
    }

    strm.EOL();
    strm.Flush();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
  }

  Status SetCoreOnFocus(Process *process, DeviceStopInfo &stop_info) {
    Status error;
    error.SetErrorToGenericError();
    if (stop_info.core_type == CoreType::AIV) {
      error = process->SetAivOnFocus(stop_info.core_id);
    } else if (stop_info.core_type == CoreType::AIC) {
      error = process->SetAicOnFocus(stop_info.core_id);
    }
    return error;
  }

  bool ShowBlockInfoDetails(Args &command, CommandReturnObject &result, AscendCommonInfo commonInfo) {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return false;
    }
    Thread *thread = m_exe_ctx.GetThreadPtr();
    if (thread == nullptr) {
      result.AppendError("Failed to get thread info");
      return false;
    }
    Status error;
 
    // 遍历并展示暂停点上下文
    strm << "Current stop state of all blocks:\n\n";
    for (uint32_t i = 0; i < commonInfo.m_core_info_vec.size(); ++i) {
      DeviceStopInfo stop_info;
      process->GetDeviceStopInfoCached(stop_info);
      stop_info.core_id = commonInfo.m_core_info_vec[i].core_id;
      stop_info.core_type = commonInfo.m_core_info_vec[i].core_type;

      error = SetCoreOnFocus(process, stop_info);
      if (error.Fail()) {
        result.AppendErrorWithFormat("[Failed to get stop info on %s %d]",
                                     (stop_info.core_type == CoreType::AIV) ? "aiv" : "aic",
                                     stop_info.core_id);
        continue;
      }
 
      std::stringstream ss;
      ss << "["
         << ((commonInfo.m_stop_info.core_id == stop_info.core_id) ? "* " : "")
         << "CoreId " << stop_info.core_id
         << ", Block " << commonInfo.m_core_info_vec[i].block_id << "]"
         << std::endl;
      strm << ss.str();
      thread->RefreshStackFramePC();
      thread->GetStatus(strm, 0, 1, 1, true);
      strm.EOL();
    }
    strm.Flush();
 
    // 切回原核
    error = SetCoreOnFocus(process, commonInfo.m_stop_info);
    if (error.Fail()) {
      result.AppendErrorWithFormat("[Failed to switch back to %s %d]",
                                   (commonInfo.m_stop_info.core_type == CoreType::AIV) ? "aiv" : "aic",
                                    commonInfo.m_stop_info.core_id
      );
      return false;
    }
    thread->RefreshStackFramePC();
 
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
    return error.Success();
  }
 
private:
  OptionGroupAscendInfoBlocks m_blocks_options;
  OptionGroupOptions m_all_options;
};

void PrintDevtbl(const SummaryInfo& summary_info, const DeviceStopInfo &stop_info,
                 const vector<CoreInfo> &core_infos, Stream &strm) {
  strm << "  CoreId  CoreType        PC         DeviceId    ChipType\n";
  static constexpr uint32_t ID_WIDTH = 4;
  for (const auto &core_info: core_infos) {
    std::stringstream ss; 
    ss << (core_info.core_id == stop_info.core_id ? " *" : "  ");
    ss << std::setw(ID_WIDTH) << static_cast<uint64_t>(core_info.core_id) << "       " <<
        (core_info.core_type == CoreType::AIC ? "AIC" : "AIV") << "    0x" << std::hex <<
        core_info.pc << "    " << std::setw(ID_WIDTH) << std::dec <<
        summary_info.dev_id << "        " <<
      DevdrvChipTypeToStr[summary_info.chip_type] << std::endl;
    strm << ss.str();
  } 
}

void PrintTensorShape(GlobalDataType data_type, const GlobalMemInfo &mem_info, std::stringstream &ss) {
  if (data_type == GlobalDataType::INVALID_TENSOR || data_type == GlobalDataType::GENERAL_TENSOR ||
      data_type == GlobalDataType::INPUT_TENSOR || data_type == GlobalDataType::OUTPUT_TENSOR) {
    auto size = mem_info.shape.dim >= 8 ? 8 : mem_info.shape.dim;
    if (size == 0) {
      ss << "       NA";
    } else {
      ss << "       [";
      for (size_t j = 0; j < size; ++j) {
        ss << mem_info.shape.dim_size[j];
        if (j < size - 1) {
          ss << ", ";
        } else if (mem_info.shape.dim > 8) {
          ss << "...";
        }
      }
      ss << "]";
    }
  } else {
    ss << "       NA";
  }
}

void PrintDeviceInfo(const SummaryInfo& summary_info, Stream &strm) {
  strm << "\n  Id           DataType                   MemType                "
            "     Addr                       Size             CoreId    "
            "CoreType    Dim\n";
  uint64_t id = 0;
  for (const auto &pair : summary_info.global_mems) {
    GlobalDataType global_data_type = pair.first;
    vector<GlobalMemory> global_mem_vec = pair.second;
    for (size_t i = 0; i < global_mem_vec.size(); ++i) {
      std::stringstream ss;
      static constexpr uint32_t WIDTH_SHORT = 4;
      static constexpr uint32_t WIDTH_LONG = 25;
      // The core_id from core file is unique for aic and aiv
      CoreIDType core_id = global_mem_vec[i].global_mem_info.coreInfo.coreId;
      CoreType core_type = CoreType::AIC;
      if (core_id >= GetMaxAicID(summary_info.chip_type)) {
          core_id -= GetMaxAicID(summary_info.chip_type);
          core_type = CoreType::AIV;
      }
      ss << std::setw(WIDTH_SHORT) << id <<
            "  " << std::setw(WIDTH_LONG) << CenterText(GlobalDataTypeToStr[global_data_type], WIDTH_LONG) <<
            "  " << std::setw(WIDTH_LONG) << CenterText(GlobalDataTypeToMemTypeStr[global_data_type], WIDTH_LONG) <<
            "  " << std::setw(WIDTH_LONG) <<
            CenterText(global_mem_vec[i].global_mem_info.addr, WIDTH_LONG, true, global_mem_vec[i].valid) <<
            "  " << std::setw(WIDTH_LONG) <<
            CenterText(global_mem_vec[i].global_mem_info.size, WIDTH_LONG, false, true);
      if (global_data_type == GlobalDataType::STACK) {
        ss << "  " << std::setw(WIDTH_SHORT) <<
               CenterText(core_id, WIDTH_SHORT, false, true) <<
              "        " << std::setw(WIDTH_SHORT) <<
              CenterText(core_type == CoreType::AIV ? "AIV" : "AIC", WIDTH_SHORT);
      } else {
        ss << "  " << std::setw(WIDTH_SHORT) << CenterText("NA", WIDTH_SHORT) <<
            "        " << std::setw(WIDTH_SHORT) << CenterText("NA", WIDTH_SHORT);
      }
      PrintTensorShape(global_data_type, global_mem_vec[i].global_mem_info, ss);
      ss << std::endl;
      strm << ss.str();
      id += 1;
    }
  }
}

class CommandObjectAscendInfoSummary : public CommandObjectParsed {
public:
  explicit CommandObjectAscendInfoSummary(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend info summary",
                            "show coredump overall info.  "
                            "",
                            "ascend info summary",
                            eCommandRequiresTarget | eCommandProcessMustBePaused |
                            eCommandProcessMustBeCoredump) {}

  ~CommandObjectAscendInfoSummary() override = default;

protected:

  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    }
    vector<CoreInfo> core_infos;
    Status error = process->GetCoresInfo(core_infos);
    if (error.Fail()) {
      result.AppendErrorWithFormatv("Get core_infos failed: {0}", error);
      return;
    }
    const auto& summary_info = process->GetSummaryInfo();
    DeviceStopInfo stop_info;
    process->GetDeviceStopInfoCached(stop_info);
    PrintDevtbl(summary_info, stop_info, core_infos, strm);
    PrintDeviceInfo(summary_info, strm);

    strm.EOL();
    strm.Flush();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
  }
};

// CommandObjectAscendInfo

class CommandObjectAscendInfo : public CommandObjectMultiword {
public:
  explicit CommandObjectAscendInfo(CommandInterpreter &interpreter)
      : CommandObjectMultiword(interpreter, "info",
                               "Show info for ascend devices tasks or blocks.") {
    LoadSubCommand("devices", CommandObjectSP(new CommandObjectAscendInfoDevices(interpreter)));
    LoadSubCommand("cores", CommandObjectSP(new CommandObjectAscendInfoAicores(interpreter)));
    LoadSubCommand("tasks", CommandObjectSP(new CommandObjectAscendInfoTasks(interpreter)));
    LoadSubCommand("stream", CommandObjectSP(new CommandObjectAscendInfoStream(interpreter)));
    LoadSubCommand("blocks", CommandObjectSP(new CommandObjectAscendInfoBlocks(interpreter)));
    LoadSubCommand("summary", CommandObjectSP(new CommandObjectAscendInfoSummary(interpreter)));
  }

  ~CommandObjectAscendInfo() override = default;
};

bool IsMatched(const std::vector<CoreInfo>& core_info_vec,
               bool coredump_mode, CoreInfo& matched_core_info, CommandReturnObject &result) {
  bool is_type_matched = false;
  bool is_id_matched = false;
  for (const auto &info : core_info_vec) {
    if (info.core_type == matched_core_info.core_type) {
      is_type_matched = true;
      if (info.core_id == matched_core_info.core_id) {
        is_id_matched = true;
        matched_core_info = info;
        break;
      }
    }
  }
  string check_command_name = "ascend info cores";
  if (coredump_mode) {
    check_command_name = "ascend info summary";
  }
  if (!is_type_matched) {
    result.AppendErrorWithFormatv("Invalid core type, please check command: {0}", check_command_name);
    return false;
  }
  if (!is_id_matched) {
    result.AppendErrorWithFormatv("Invalid core id, please check command: {0}", check_command_name);
    return false;
  }
  return true;
}
// CommandObjectAscendAic

class CommandObjectAscendAic : public CommandObjectParsed {
public:
  explicit CommandObjectAscendAic(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend aic",
                            "change the id of the focused ascend aicore.  "
                            "",
                            "ascend aic <id>",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDeviceOrCoredump) {
    CommandArgumentEntry arg;
    CommandArgumentData aic_idx_arg;

    aic_idx_arg.arg_type = eArgTypeAscendCoreIndex;
    aic_idx_arg.arg_repetition = eArgRepeatPlain;
    arg.push_back(aic_idx_arg);
    m_arguments.push_back(arg);
  }

  ~CommandObjectAscendAic() override = default;

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    } else if (command.GetArgumentCount() != 1) {
      result.AppendErrorWithFormat(
          "'%s' takes exactly one aic index argument:\nUsage: %s\n",
          m_cmd_name.c_str(), m_cmd_syntax.c_str());
      return;
    }
    Thread *thread = m_exe_ctx.GetThreadPtr();
    if (thread == nullptr) {
      result.AppendError("Failed to get thread info");
      return;
    }
    if(!CheckStringValid(std::string(command.GetArgumentAtIndex(0)), result)) return;
    uint32_t index_id;
    if (!llvm::to_integer(command.GetArgumentAtIndex(0), index_id)) {
      result.AppendErrorWithFormat("Invalid aicore index '%s'", command.GetArgumentAtIndex(0));
      return;
    }
    std::vector<CoreInfo> core_info_vec;
    Status error = process->GetCoresInfo(core_info_vec);
    if (error.Fail()) {
      result.AppendErrorWithFormat("Failed to get cores info.");
      return;
    }
    CoreInfo matched_core_info;
    matched_core_info.core_id = index_id;
    matched_core_info.core_type = CoreType::AIC;
    if (!IsMatched(core_info_vec, process->DeviceCoredumpEnable(), matched_core_info, result)) {
      return;
    }
    error = process->SetAicOnFocus(index_id);
    if (error.Fail()) {
      result.AppendErrorWithFormat("Failed to switch focus to ascend aicore %d", index_id);
      return;
    }
    // update core info cache in client
    DeviceStopInfo stop_info;
    process->GetDeviceStopInfoCached(stop_info);
    stop_info.core_id = matched_core_info.core_id;
    stop_info.core_type = matched_core_info.core_type;
    process->SetDeviceStopInfoCached(stop_info);
    ShowInfoWhenStopped(*process, *thread, strm);
  }
};

// CommandObjectAscendAiv

class CommandObjectAscendAiv : public CommandObjectParsed {
public:
  explicit CommandObjectAscendAiv(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend aiv",
                            "change the id of the focused ascend aivector.  "
                            "",
                            "ascend aiv <id>",
                            eCommandRequiresProcess | eCommandTryTargetAPILock |
                            eCommandProcessMustBeLaunched | eCommandProcessMustBePaused |
                            eCommandProcessMustBePausedInDeviceOrCoredump) {
    CommandArgumentEntry arg;
    CommandArgumentData aic_idx_arg;

    aic_idx_arg.arg_type = eArgTypeAscendCoreIndex;
    aic_idx_arg.arg_repetition = eArgRepeatPlain;
    arg.push_back(aic_idx_arg);
    m_arguments.push_back(arg);
  }

  ~CommandObjectAscendAiv() override = default;

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Stream &strm = result.GetOutputStream();
    result.SetStatus(eReturnStatusSuccessFinishNoResult);
    Process *process = m_exe_ctx.GetProcessPtr();
    if (process == nullptr) {
      result.AppendError("Failed to get process info");
      return;
    } else if (command.GetArgumentCount() != 1) {
      result.AppendErrorWithFormat(
          "'%s' takes exactly one aiv index argument:\nUsage: %s\n",
          m_cmd_name.c_str(), m_cmd_syntax.c_str());
      return;
    }
    Thread *thread = m_exe_ctx.GetThreadPtr();
    if (thread == nullptr) {
      result.AppendError("Failed to get thread info");
      return;
    }
    if(!CheckStringValid(std::string(command.GetArgumentAtIndex(0)), result)) return;
    uint32_t index_id;
    if (!llvm::to_integer(command.GetArgumentAtIndex(0), index_id)) {
      result.AppendErrorWithFormat("Invalid aivector index '%s'", command.GetArgumentAtIndex(0));
      return;
    }
    std::vector<CoreInfo> core_info_vec;
    CoreInfo matched_core_info;
    Status error = process->GetCoresInfo(core_info_vec);
    if (error.Fail()) {
      result.AppendErrorWithFormat("Failed to get cores info.");
      return;
    }
    matched_core_info.core_id = index_id;
    matched_core_info.core_type = CoreType::AIV;
    if (!IsMatched(core_info_vec, process->DeviceCoredumpEnable(), matched_core_info, result)) {
      return;
    }
    error = process->SetAivOnFocus(index_id);
    if (error.Fail()) {
      result.AppendErrorWithFormat("Failed to switch focus to ascend vectorcore %d", index_id);
      return;
    }
    // update core info cache in client
    DeviceStopInfo stop_info;
    process->GetDeviceStopInfoCached(stop_info);
    stop_info.core_id = matched_core_info.core_id;
    stop_info.core_type = matched_core_info.core_type;
    process->SetDeviceStopInfoCached(stop_info);
    ShowInfoWhenStopped(*process, *thread, strm);
  }
};

// CommandObjectAscendDevice

class CommandObjectAscendDevice : public CommandObjectParsed {
public:
  explicit CommandObjectAscendDevice(CommandInterpreter &interpreter)
      : CommandObjectParsed(interpreter, "ascend device",
                            "change the id of the focused ascend device.  "
                            "",
                            "ascend device <id>",
                            eCommandRequiresTarget) {
    CommandArgumentEntry arg;
    CommandArgumentData aic_idx_arg;

    aic_idx_arg.arg_type = eArgTypeAscendCoreIndex;
    aic_idx_arg.arg_repetition = eArgRepeatPlain;
    arg.push_back(aic_idx_arg);
    m_arguments.push_back(arg);
  }

  ~CommandObjectAscendDevice() override = default;

protected:
  void DoExecute(Args &command, CommandReturnObject &result) override {
    Debugger &debugger = GetDebugger();
    Target *target = debugger.GetSelectedTarget().get();
    if (target == nullptr) {
      result.AppendError("Failed to get target info");
      return;
    } else if (command.GetArgumentCount() != 1) {
      result.AppendErrorWithFormat(
          "'%s' takes exactly one aic index argument:\nUsage: %s\n",
          m_cmd_name.c_str(), m_cmd_syntax.c_str());
      return;
    }
    if(!CheckStringValid(std::string(command.GetArgumentAtIndex(0)), result)) return;
    int32_t index_id;
    if (!llvm::to_integer(command.GetArgumentAtIndex(0), index_id)) {
      result.AppendErrorWithFormat("Invalid device index '%s'", command.GetArgumentAtIndex(0));
      return;
    }
    target->SetDeviceId(index_id);
  }
};

// CommandObjectMultiwordAscend

CommandObjectMultiwordAscend::CommandObjectMultiwordAscend(
    CommandInterpreter &interpreter)
    : CommandObjectMultiword(interpreter, "ascend",
                             "Commands for operating on "
                             "ascend device in "
                             "the current process.",
                             "ascend <subcommand> [<subcommand-options>]") {
  CommandObjectSP info_command_object(
      new CommandObjectAscendInfo(interpreter));
  info_command_object->SetCommandName("ascend info");

  LoadSubCommand("info", info_command_object);

  LoadSubCommand("aic",
                 CommandObjectSP(new CommandObjectAscendAic(interpreter)));
  LoadSubCommand("aiv",
                 CommandObjectSP(new CommandObjectAscendAiv(interpreter)));
  LoadSubCommand("device",
                 CommandObjectSP(new CommandObjectAscendDevice(interpreter)));
}

CommandObjectMultiwordAscend::~CommandObjectMultiwordAscend() = default;
#endif
