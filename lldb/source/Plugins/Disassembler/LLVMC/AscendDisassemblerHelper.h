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

#ifndef liblldb_AscendDisassemblerHelper_H_
#define liblldb_AscendDisassemblerHelper_H_
#ifdef MS_DEBUGGER

#include "lldb/Utility/MessageDefines.h"
#include "lldb/Utility/ArchSpec.h"

#include <string>
#include <vector>

namespace lldb_private {
class Address;
class AddressRange;
class Status;
class AscendDisassemblerHelper {
public:
  static void GetInstruction(const uint8_t *opcode_data,
                             const size_t opcode_data_len,
                             const lldb_private::ArchSpec &arch,
                             uint64_t &flags, uint64_t &inst_size);
};

class AscendDisassembler {
public:
  virtual void GetInstruction(const uint8_t *opcode_data,
                              const size_t opcode_data_len, uint64_t &flags,
                              const InterruptPosType pos_type,
                              uint64_t &inst_size) = 0;
};

// 离线 objdump 文本查表，在首次 Address 查询时从实际 Module 的 ELF 懒加载。
// 外层缓存统一按完整设备 ELF 的 SHA256 分表，与 kernel 名和加载来源无关。
// 查询通过完整 Address 的所属 Module 取得 ELF SHA256，表缺失时才生成。
// 表内的键 = FileAddress（与 objdump 输出地址天然同键），
// 每条记录同时保存 objdump 机器码字段对应的实际指令长度。
// Module 析构及新表生成前清理过期关联和不再共享的表。
// 新表生成前已有缓存超过 64 MiB 时按 LRU 降至阈值以下。
// 不考虑即将生成的新表大小，插入后允许暂时超限。
// 返回空串 / false / 0 = 未命中，解析阶段不得据此猜测指令边界。
class AscendObjdumpText {
public:
  // 由 GetModuleTableKey 在查询入口已持缓存锁时调用，本函数不再自行加锁。
  // 通过 stdin 将 ELF 交给 llvm-objdump -d -，
  // 不创建临时文件；PipeCall 以写线程发送 ELF，主线程读取输出。
  // 内部计算完整 ELF 的 SHA256；相同内容重复调用直接复用缓存，不重新启动
  // objdump。建表前的清理即使在后续生成失败时也生效；查询不回退到其他表。
  // 工具路径为 ASCEND_HOME_PATH/bin/llvm-objdump。
  // 未设置环境变量或工具执行失败时报告错误。
  // 返回 false 表示生成失败（工具不存在 / 执行失败 / 输出不可解析）。
  static bool GenerateFromELF(const char *elf_bytes, size_t elf_size);
  static std::string Lookup(const Address &address);
  // address 是否是其所属模块中某条真实指令的起始地址。
  static bool Contains(const Address &address);
  // 返回该地址在 objdump 表中记录的实际指令长度，未命中返回 0。
  static uint64_t GetInstructionSize(const Address &address);
  // 清理过期 Module 关联，仅删除没有其他存活关联的表。
  // 其他线程占用缓存锁时，延后到下一次析构或建表清理。
  static void RemoveExpiredModules();
  // 按范围复制真实指令地址，不读取设备内存或解码指令。
  // 范围字节数为 0 时，从起点最多返回 max_instructions 条。
  static Status GetInstructionAddresses(const AddressRange &range,
                                        size_t max_instructions,
                                        std::vector<Address> &addresses);
};
} // namespace lldb_private

#endif
#endif // #ifndef liblldb_AscendDisassemblerHelper_H_
