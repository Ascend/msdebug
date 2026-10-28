/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifdef MS_DEBUGGER

#include "AscendDisassemblerHelper.h"

#include "AscendDisassembler310P.h"
#include "AscendDisassembler910B.h"
#include "AscendDisassembler950.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "lldb/Core/Address.h"
#include "lldb/Core/AddressRange.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Utility/Args.h"
#include "lldb/Utility/FileSpec.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/Status.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SHA256.h"

using namespace lldb_private;
using namespace std;

void AscendDisassemblerHelper::GetInstruction(const uint8_t *opcode_data,
                                              const size_t opcode_data_len,
                                              const ArchSpec &arch,
                                              uint64_t &flags,
                                              uint64_t &inst_size) {
  shared_ptr<AscendDisassembler> disa;
  switch (arch.GetSocType()) {
    case SocType::ASCEND310P:
      disa = make_shared<AscendDisassembler310P>();
      break;
    case SocType::ASCEND910B:
      disa = make_shared<AscendDisassembler910B>();
      break;
    case SocType::ASCEND950:
      disa = make_shared<AscendDisassembler950>();
      break;
    default:
      flags = 0;
      inst_size = 4;
      return;
  }
  disa->GetInstruction(opcode_data, opcode_data_len, flags, arch.GetPosType(), inst_size);
}

// 离线 objdump 文本查表
// 数据源：内存中的 kernel ELF 经 CANN llvm-objdump -d - 生成的文本。
// 键 = 指令的 FileAddress——与 objdump 输出的地址列天然同键

namespace {
struct ObjdumpInstruction {
  std::string text;
  uint64_t size = 0;
};

// 一张表：FileAddress -> 指令文本和实际机器码长度。
using ObjdumpTable = std::map<uint64_t, ObjdumpInstruction>;

struct CachedObjdumpTable {
  ObjdumpTable instructions;
  size_t memory_bytes = 0;
  uint64_t last_used = 0;
};
using ObjdumpTables = std::map<std::string, CachedObjdumpTable>;
constexpr size_t kObjdumpCacheLimit = 64 * 1024 * 1024;

// 任一缓存静态对象开始销毁后，后续 Module 析构不再访问缓存。
std::atomic<bool> g_objdump_cache_destroying{false};
struct ObjdumpCacheShutdownGuard {
  ~ObjdumpCacheShutdownGuard() { g_objdump_cache_destroying.store(true); }
};

// 所有表：完整设备 ELF 的 SHA256 -> 表，由 GenerateFromELF 统一计算键。
// 活调试与 coredump 使用同一规则，不依赖 kernel 名或加载来源。
// 不同 ELF 可有相同的指令地址；同一 ELF 内的多个 kernel 和 helper 共用一张表。
// Module 析构及建表前清理过期表；超限的旧表仍仅在建表前淘汰。
ObjdumpTables &GetObjdumpTables() {
  static ObjdumpTables tables;
  // 后构造、先析构，在容器销毁前标记退出阶段。
  static ObjdumpCacheShutdownGuard shutdown_guard;
  return tables;
}

uint64_t &GetObjdumpAccessSequence() {
  static uint64_t sequence = 0;
  return sequence;
}

// 保守估算节点、文本容量和分配开销，不包含 ELF、工具输出或 keys。
size_t EstimateTableMemory(const std::string &key, const ObjdumpTable &table) {
  constexpr size_t node_overhead = 4 * sizeof(void *) + 2 * sizeof(size_t);
  size_t bytes =
      sizeof(ObjdumpTables::value_type) + node_overhead + key.capacity() + 1;
  for (const auto &entry : table)
    bytes += sizeof(ObjdumpTable::value_type) + node_overhead +
             entry.second.text.capacity() + 1;
  return bytes;
}

// 建表时把每条的操作数前补空格，使整个表的操作数列对齐。
//
// 背景：上游 Instruction::Dump（lldb/source/Core/Disassembler.cpp:610,
// 665-671）的 助记符列宽是**每条指令各自算**的局部变量——初始
// 7，遇到更长的助记符就改成 长度+1。这在 x86/arm 上无害（助记符普遍 <= 7
// 字符），但 hiipu64 助记符跨 3~12 字符（MOV / MOVK / ADD.s64 /
// SBITSET1.b64），每条各放宽一次， 操作数起始列就各不相同，输出参差不齐。
//
// 做法：在表里预先把操作数前补空格，补齐的总量 = 统一列宽 - 本条实际列宽，
// 于是打印时"补齐 + 预补空格"合起来正好等于统一列宽，整列就对齐了。
// 注意必须补在操作数侧而非助记符侧：DisassemblerLLVMC.cpp:657 用
// text.find(' ') 切分助记符与操作数，助记符里出现空格会被截断。
//
// 仅对 hiipu64 生效（该表只有 hiipu64 会查）。
void AlignOperandsColumn(ObjdumpTable &table) {
  // 与上游 Instruction::Dump 保持一致的两个常量。
  constexpr size_t kBaseOpcodeColumnWidth = 7;
  // 本条指令在上游逻辑下实际占用的列宽。
  auto field_width = [kBaseOpcodeColumnWidth](size_t mnemonic_len) {
    return mnemonic_len >= kBaseOpcodeColumnWidth ? mnemonic_len + 1
                                                  : kBaseOpcodeColumnWidth;
  };

  size_t uniform_width = kBaseOpcodeColumnWidth;

  // 第一遍：剥掉操作数侧的前导空白，同时统计统一列宽。
  // 这一步不能省——objdump 用「助记符 + 空格/制表符」做助记符对齐，
  // 操作数里会残留长度不一的前导空白：
  //   "MOV \t\tX29, #32672"   → 2 个
  //   "AND.b64  \tX15, ..."  → 2 个
  //   "CMP.LT.u64\tX1, X4"    → 0 个
  //   "BAR.V <_Z...>"         → 0 个
  // 不先剥掉的话，这些残留会叠加到补齐量上，整列仍然参差。
  for (auto &entry : table) {
    std::string &text = entry.second.text;
    const size_t sp = text.find(' ');
    if (sp == std::string::npos) {
      uniform_width = std::max(uniform_width, field_width(text.size()));
      continue;
    }
    uniform_width = std::max(uniform_width, field_width(sp));
    const size_t op_start = text.find_first_not_of(' ', sp);
    if (op_start == std::string::npos)
      text.erase(sp); // 只有助记符，无操作数
    else
      text = text.substr(0, sp + 1) + text.substr(op_start);
  }

  // 第二遍：按统一列宽在操作数前补空格。
  for (auto &entry : table) {
    std::string &text = entry.second.text;
    const size_t sp = text.find(' ');
    if (sp == std::string::npos)
      continue; // 无操作数，无需补齐
    const size_t pad = uniform_width - field_width(sp);
    text.insert(sp + 1, pad, ' ');
  }
}

// 弱引用不延长 Module 生命周期，owner 身份也避免指针地址复用造成误命中。
using ModuleTableKeys =
    std::map<lldb::ModuleWP, std::string, std::owner_less<lldb::ModuleWP>>;

ModuleTableKeys &GetModuleTableKeys() {
  static ModuleTableKeys keys;
  static ObjdumpCacheShutdownGuard shutdown_guard;
  return keys;
}

std::recursive_mutex &GetObjdumpTextMutex() {
  static std::recursive_mutex m;
  static ObjdumpCacheShutdownGuard shutdown_guard;
  return m;
}

// 调用者持有缓存锁；不访问 Module，只检查 weak_ptr 是否过期。
void RemoveExpiredModulesLocked() {
  auto &keys = GetModuleTableKeys();
  auto &tables = GetObjdumpTables();
  std::set<std::string> expired_keys;
  for (auto it = keys.begin(); it != keys.end();) {
    if (it->first.expired()) {
      expired_keys.insert(it->second);
      it = keys.erase(it);
    } else {
      ++it;
    }
  }
  for (const auto &key : expired_keys) {
    const bool shared =
        std::any_of(keys.begin(), keys.end(), [&](const auto &entry) {
          return entry.second == key && !entry.first.expired();
        });
    if (!shared)
      tables.erase(key);
  }
}

// 调用者持有 GetObjdumpTextMutex()，并保持到后续指令查询及结果复制结束。
std::string GetModuleTableKey(const Address &address) {
  const lldb::ModuleSP module = address.GetModule();
  if (!module)
    return {};
  const lldb::ModuleWP owner(module);
  auto &keys = GetModuleTableKeys();
  auto &tables = GetObjdumpTables();
  const auto found = keys.find(owner);
  if (found != keys.end()) {
    auto table = tables.find(found->second);
    if (table != tables.end()) {
      table->second.last_used = ++GetObjdumpAccessSequence();
      return found->second;
    }
  }

  ObjectFile *object = module->GetObjectFile();
  if (!object)
    return {};
  DataExtractor data;
  const size_t size = object->GetByteSize();
  if (size == 0 || object->GetData(0, size, data) != size)
    return {};
  const std::string key = llvm::toHex(
      llvm::SHA256::hash(llvm::ArrayRef<uint8_t>(data.GetDataStart(), size)),
      /*LowerCase=*/true);

  auto table = tables.find(key);
  if (table != tables.end()) {
    table->second.last_used = ++GetObjdumpAccessSequence();
    keys.emplace(owner, key);
    return key;
  }
  // 首次查询才建表，复用调用者持有的锁；失败后允许再次查询重试。
  if (!AscendObjdumpText::GenerateFromELF(
          reinterpret_cast<const char *>(data.GetDataStart()), size))
    return {};

  keys.emplace(owner, key);
  return key;
}

// 调用者持有表锁；指针只在锁的作用域内使用。
const ObjdumpInstruction *FindInstructionLocked(const std::string &key,
                                                uint64_t file_addr) {
  const auto &tables = GetObjdumpTables();
  const auto table = tables.find(key);
  if (table == tables.end())
    return nullptr;
  const auto instruction = table->second.instructions.find(file_addr);
  return instruction == table->second.instructions.end() ? nullptr
                                                         : &instruction->second;
}

// 行格式：[空白]十六进制地址: [空白]十六进制字节 [空白]助记符 [操作数]
// 非匹配行（函数头 "X():"、"; 源码行"、空行）一律跳过
bool ParseObjdumpLine(const std::string &line, uint64_t &addr,
                      ObjdumpInstruction &instruction) {
  const size_t colon = line.find(':');
  if (colon == std::string::npos)
    return false;
  const size_t addr_start = line.find_first_not_of(" \t");
  if (addr_start == std::string::npos || addr_start >= colon)
    return false;
  const std::string addr_str = line.substr(addr_start, colon - addr_start);
  if (addr_str.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
    return false; // 地址段含非 hex：函数头/路径/源码行
  addr = std::stoull(addr_str, nullptr, 16);

  size_t pos = line.find_first_not_of(" \t", colon + 1);
  if (pos == std::string::npos)
    return false;
  const size_t bytes_start = pos;
  // 跳过整段字节字段（不能按 hex 字符集截断——CALL/END 等助记符以 hex 字母开头）
  pos = line.find_first_not_of("0123456789abcdefABCDEF", pos);
  if (pos == bytes_start)
    return false;
  const size_t hex_digits =
      pos == std::string::npos ? line.size() - bytes_start : pos - bytes_start;
  if (hex_digits % 2 != 0 || hex_digits / 2 > 32 || pos == std::string::npos ||
      (line[pos] != ' ' && line[pos] != '\t'))
    return false;
  // 直接记录本条机器码的长度，函数末尾或不连续地址无需依赖下一条指令。
  instruction.size = hex_digits / 2;
  pos = line.find_first_not_of(" \t", pos);
  if (pos == std::string::npos)
    return false;
  std::string &text = instruction.text;
  text = line.substr(pos);
  // objdump 用制表符对齐助记符与操作数（如 "MOV\t\tX15, SYS_VA_BASE"），
  // 原样保留会让输出参差不齐，且干扰 AlignOperandsColumn 的列宽计算
  // （助记符长度按首个空格切分，制表符会被算进助记符里）。统一折叠成单空格。
  for (char &ch : text) {
    if (ch == '\t' || ch == '\r')
      ch = ' ';
  }
  const size_t last = text.find_last_not_of(" \t\r");
  if (last != std::string::npos)
    text = text.substr(0, last + 1);
  return !text.empty();
}

} // namespace

std::string AscendObjdumpText::Lookup(const Address &address) {
  const uint64_t file_addr = address.GetFileAddress();
  std::lock_guard<std::recursive_mutex> lock(GetObjdumpTextMutex());
  const std::string key = GetModuleTableKey(address);
  const auto *instruction = FindInstructionLocked(key, file_addr);
  return instruction ? instruction->text : std::string();
}

bool AscendObjdumpText::Contains(const Address &address) {
  const uint64_t file_addr = address.GetFileAddress();
  std::lock_guard<std::recursive_mutex> lock(GetObjdumpTextMutex());
  const std::string key = GetModuleTableKey(address);
  return FindInstructionLocked(key, file_addr) != nullptr;
}

uint64_t AscendObjdumpText::GetInstructionSize(const Address &address) {
  const uint64_t file_addr = address.GetFileAddress();
  std::lock_guard<std::recursive_mutex> lock(GetObjdumpTextMutex());
  const std::string key = GetModuleTableKey(address);
  const auto *instruction = FindInstructionLocked(key, file_addr);
  return instruction ? instruction->size : 0;
}

void AscendObjdumpText::RemoveExpiredModules() {
  if (g_objdump_cache_destroying.load())
    return;
  // 支持同线程析构重入；其他线程占锁时不等待，避免锁顺序死锁。
  std::unique_lock<std::recursive_mutex> lock(GetObjdumpTextMutex(),
                                              std::try_to_lock);
  if (lock.owns_lock())
    RemoveExpiredModulesLocked();
}

Status
AscendObjdumpText::GetInstructionAddresses(const AddressRange &range,
                                           size_t max_instructions,
                                           std::vector<Address> &addresses) {
  addresses.clear();
  const Address &base = range.GetBaseAddress();
  const lldb::ModuleSP module = base.GetModule();
  const uint64_t begin = base.GetFileAddress();
  const uint64_t bytes = range.GetByteSize();
  if (!module || begin == LLDB_INVALID_ADDRESS ||
      bytes > LLDB_INVALID_ADDRESS - begin ||
      (bytes == 0 && max_instructions == 0))
    return Status("invalid device instruction range");

  std::lock_guard<std::recursive_mutex> lock(GetObjdumpTextMutex());
  const std::string key = GetModuleTableKey(base);
  const auto found = GetObjdumpTables().find(key);
  if (found == GetObjdumpTables().end())
    return Status("unable to prepare the device instruction table");
  const auto &table = found->second.instructions;
  auto entry = table.lower_bound(begin);
  if (entry == table.end() || entry->first != begin)
    return Status(
        "selected device range does not start at an instruction boundary");

  for (; entry != table.end(); ++entry) {
    if (bytes ? entry->first - begin >= bytes
              : addresses.size() >= max_instructions)
      break;
    const uint64_t size = entry->second.size;
    if (size == 0 || size > LLDB_INVALID_ADDRESS - entry->first ||
        (bytes && size > bytes - (entry->first - begin))) {
      addresses.clear();
      return Status("device instruction exceeds the selected range");
    }
    Address address;
    if (!module->ResolveFileAddress(entry->first, address)) {
      addresses.clear();
      return Status("unable to resolve a device instruction address");
    }
    addresses.push_back(address);
  }
  return Status();
}

// 由 kernel ELF 自动生成文本表
// 背景：此前需手工跑两条命令生成中间文件——
//   llvm-objcopy -j.aicore_binary -O binary ./demo kernel.o   # 取代码段
//   llvm-objdump -Sl kernel.o > kernel.txt                    # 反汇编成文本
// 设备 ELF 已保存在 Module 中，Lookup/Contains/GetInstructionSize 首次查询时
// 通过 Address 找到实际 Module，读取完整 ELF，按内容复用或生成表：
//
//   1. 拿到内存中的 kernel ELF（本函数入参）
//   2. PipeCall 启动 llvm-objdump -d -，将 ELF 二进制写入子进程 stdin
//   3. 写线程传输 ELF，主线程读取反汇编输出，不创建临时文件或匿名内存文件
//   4. 复用 ParseObjdumpLine 解析成表，按 ELF SHA256 存表，查询时按 Module 选表
//
// 失败（CANN 工具不存在 / 执行失败 / 输出不可解析）一律返回
// false：无法查到可靠的指令边界时停止本次反汇编，不猜测指令长度。

namespace {
// 固定从 ASCEND_HOME_PATH/bin 定位 CANN 工具，不搜索 PATH 或查询版本。
Status ResolveExecutable(const std::string &name, std::string &path) {
  path.clear();
  const char *home = getenv("ASCEND_HOME_PATH");
  if (home == nullptr || home[0] == '\0')
    return Status("ASCEND_HOME_PATH is not set; source CANN set_env.sh before "
                  "starting msdebug");
  FileSpec executable(home);
  executable.AppendPathComponent("bin");
  executable.AppendPathComponent(name);
  path = executable.GetPath();
  return Status();
}

// 登记本执行器创建的管道 FD；创建、关闭和 fork 共用同一把锁。
class Pipe {
public:
  Pipe() = default;
  Pipe(const Pipe &) = delete;
  Pipe &operator=(const Pipe &) = delete;
  ~Pipe() {
    Close(m_fds[0]);
    Close(m_fds[1]);
  }

  bool Create() {
    std::lock_guard<std::mutex> guard(mtx);
    if (::pipe(m_fds) != 0)
      return false;
    s_fds.insert(m_fds[0]);
    s_fds.insert(m_fds[1]);
    return true;
  }

  int &operator[](size_t index) { return m_fds[index]; }

  static void Close(int &fd) {
    std::lock_guard<std::mutex> guard(mtx);
    if (fd >= 0) {
      ::close(fd);
      s_fds.erase(fd);
      fd = -1;
    }
  }

  // 仅在子进程完成重定向后调用；保留已经重定向的标准输入输出。
  static void CloseAll() {
    std::lock_guard<std::mutex> guard(mtx);
    for (int fd : s_fds)
      if (fd > STDERR_FILENO)
        ::close(fd);
    s_fds.clear();
  }

  static std::mutex mtx;

private:
  int m_fds[2] = {-1, -1};
  static std::set<int> s_fds;
};

std::mutex Pipe::mtx;
std::set<int> Pipe::s_fds;

bool Write(int &fd, const std::string &input) {
  size_t sent = 0;
  while (sent < input.size()) {
    const ssize_t bytes = ::write(fd, input.data() + sent, input.size() - sent);
    if (bytes < 0 && errno == EINTR)
      continue;
    if (bytes <= 0)
      break;
    sent += static_cast<size_t>(bytes);
  }
  Pipe::Close(fd); // 写完关闭管道，通知子进程输入 EOF。
  return sent == input.size();
}

bool Read(int fd, std::string &output) {
  char buffer[64 * 1024];
  for (;;) {
    const ssize_t bytes = ::read(fd, buffer, sizeof(buffer));
    if (bytes < 0 && errno == EINTR)
      continue;
    if (bytes < 0)
      return false;
    if (bytes == 0)
      return true;
    output.append(buffer, static_cast<size_t>(bytes));
  }
}

bool PipeCall(const std::vector<std::string> &cmd, std::string &output,
              const std::string &input = {}) {
  output.clear();
  if (cmd.empty())
    return false;
  Args args;
  for (const auto &arg : cmd)
    args.AppendArgument(arg);
  std::string command;
  args.GetQuotedCommandString(command);
  LLDB_LOG(GetLog(LLDBLog::Process), "PipeCall: {0}", command);
  char **argv = args.GetArgumentVector();

  Pipe pipe_stdin, pipe_stdout;
  if (!pipe_stdin.Create() || !pipe_stdout.Create())
    return false;

  pid_t pid;
  {
    std::lock_guard<std::mutex> guard(Pipe::mtx);
    pid = fork();
  }
  if (pid < 0)
    return false;
  if (pid == 0) {
    if (dup2(pipe_stdin[0], STDIN_FILENO) < 0 ||
        dup2(pipe_stdout[1], STDOUT_FILENO) < 0 ||
        dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
      _exit(EXIT_FAILURE);
    Pipe::CloseAll();
    execvp(argv[0], argv);
    _exit(EXIT_FAILURE);
  }

  Pipe::Close(pipe_stdin[0]);
  Pipe::Close(pipe_stdout[1]);
  bool write_ok = false;
  std::thread writer([&] { write_ok = Write(pipe_stdin[1], input); });
  const bool read_ok = Read(pipe_stdout[0], output);
  Pipe::Close(pipe_stdout[0]);

  int status = 0;
  pid_t waited;
  do {
    waited = waitpid(pid, &status, 0);
  } while (waited < 0 && errno == EINTR);
  writer.join();
  return read_ok && write_ok && waited == pid && WIFEXITED(status) &&
         WEXITSTATUS(status) == 0;
}

} // namespace

bool AscendObjdumpText::GenerateFromELF(const char *elf_bytes,
                                        size_t elf_size) {
  Log *log = GetLog(LLDBLog::Process);
  std::string table_key;
  {
    // 与 GetModuleTableKey 一样，整个函数由查询入口持锁保护。
    auto &keys = GetModuleTableKeys();
    auto &tables = GetObjdumpTables();
    RemoveExpiredModulesLocked();

    size_t bytes = 0;
    for (const auto &entry : tables)
      bytes += entry.second.memory_bytes;
    // 只考察已有表：超限时降到阈值以下，不预留新表，也不在插入后再淘汰。
    if (bytes > kObjdumpCacheLimit) {
      while (!tables.empty() && bytes >= kObjdumpCacheLimit) {
        auto oldest = std::min_element(
            tables.begin(), tables.end(), [](const auto &a, const auto &b) {
              return a.second.last_used < b.second.last_used;
            });
        for (auto it = keys.begin(); it != keys.end();) {
          if (it->second == oldest->first)
            it = keys.erase(it);
          else
            ++it;
        }
        bytes -= oldest->second.memory_bytes;
        tables.erase(oldest);
      }
    }
    if (elf_bytes == nullptr || elf_size == 0) {
      LLDB_LOG(log, "GenerateFromELF: empty elf, skip");
      return false;
    }
    const auto elf = llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(elf_bytes), elf_size);
    table_key = llvm::toHex(llvm::SHA256::hash(elf), /*LowerCase=*/true);
    // 同一份 ELF 重复加载时不重新启动 objdump，与 kernel 名和加载来源无关。
    auto found = tables.find(table_key);
    if (found != tables.end()) {
      found->second.last_used = ++GetObjdumpAccessSequence();
      LLDB_LOG(log, "objdump text table reused: key={0}", table_key);
      return true;
    }
  }

  std::string objdump;
  Status error = ResolveExecutable("llvm-objdump", objdump);
  if (error.Fail()) {
    Debugger::ReportError(error.AsCString());
    return false;
  }
  std::string output;
  if (!PipeCall({objdump, "-d", "-"}, output,
                std::string(elf_bytes, elf_size)) ||
      output.empty()) {
    error.SetErrorStringWithFormat("CANN llvm-objdump failed: %s",
                                   objdump.c_str());
    Debugger::ReportError(error.AsCString());
    LLDB_LOG(log, "GenerateFromELF: objdump failed (cmd={0}, output={1})",
             objdump, output);
    return false;
  }

  ObjdumpTable table;
  for (llvm::StringRef rest(output.data(), output.size()); !rest.empty();) {
    const size_t newline = rest.find('\n');
    llvm::StringRef line =
        rest.substr(0, newline == llvm::StringRef::npos ? rest.size() : newline)
            .trim();
    if (!line.empty()) {
      uint64_t addr = 0;
      ObjdumpInstruction instruction;
      if (ParseObjdumpLine(line.str(), addr, instruction))
        table[addr] = std::move(instruction);
    }
    if (newline == llvm::StringRef::npos)
      break;
    rest = rest.substr(newline + 1);
  }
  if (table.empty()) {
    LLDB_LOG(log, "GenerateFromELF: objdump produced no parseable lines");
    return false;
  }
  AlignOperandsColumn(table);

  auto &tables = GetObjdumpTables();
  auto inserted =
      tables.emplace(table_key, CachedObjdumpTable{std::move(table), 0, 0});
  if (inserted.second)
    inserted.first->second.memory_bytes = EstimateTableMemory(
        inserted.first->first, inserted.first->second.instructions);
  inserted.first->second.last_used = ++GetObjdumpAccessSequence();
  LLDB_LOG(log, "objdump text table generated: key={0}, entries={1}", table_key,
           inserted.first->second.instructions.size());
  return true;
}
#endif
