/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "lldb/Host/Host.h"
#include "lldb/Host/HostInfo.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "Plugins/Process/Utility/LinuxProcMaps.h"
#include "Plugins/Process/gdb-remote/GDBRemoteCommunicationServerLLGS.h"
#include "Plugins/Process/gdb-remote/ProcessGDBRemoteLog.h"
#define private public  // hack complier
#define protected public
#include "Plugins/Process/Linux/AscendProcessLinux.h"
#include "Plugins/Process/Linux/AscendThreadLinux.h"
#undef private
#undef protected
#include "Plugins/Disassembler/LLVMC/AscendDisassembler950.h"
#include "TestingSupport/Plugins/AscendProcessLinuxTestUtils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "llvm/Testing/Support/Error.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::process_linux;
using namespace lldb_private::process_gdb_remote;
using namespace ::testing;

class FakeDeviceContext : public DeviceContext {
public:
    FakeDeviceContext() : DeviceContext(0, 0, 0) {
        m_drv_fd = 3;
    }

    Status Init() override { return Status(); }
    bool StartListenThread() override { return true; }
    Status EnableDebugMode() override { return Status(); }
    Status CheckRegisterAddr(CoreType core_type, uint64_t addr) const override {
        return Status();
    }
    SocType GetSocType() override { return SocType::SOC_END; }
    MemType GetStackMemType() const override { return MemType::OUT_MEM; }

    size_t ReadGlobalMemory(lldb::addr_t addr, size_t size, void *data) override {
        DebugInfo debug_info = {0, 1000, 0, 0};
        DmaParam *param = (DmaParam*)debug_info.data;
        std::vector<uint8_t> tmpData(size, 0);
        param->host_addr = (uint64_t)&tmpData[0];
        param->device_addr = addr;
        param->size = size;
        param->direction = DEVDRV_DMA_DEVICE_TO_HOST;
        int32_t rtn = ioctl(m_drv_fd, CMD_GM_COPY, &debug_info);
        if (rtn != 0) return 0;
        std::copy(tmpData.begin(), tmpData.end(), static_cast<uint8_t*>(data));
        return size;
    }

    size_t WriteGlobalMemory(lldb::addr_t addr, size_t size,
                             const void *data) override {
        DebugInfo debug_info = {0, 1000, 0, 0};
        DmaParam *param = (DmaParam*)debug_info.data;
        uint8_t *data_ptr = static_cast<uint8_t *>(const_cast<void *>(data));
        std::vector<uint8_t> tmpData(data_ptr, data_ptr + size);
        param->host_addr = (uint64_t)&tmpData[0];
        param->device_addr = addr;
        param->size = size;
        param->direction = DEVDRV_DMA_HOST_TO_DEVICE;
        int32_t rtn = ioctl(m_drv_fd, CMD_GM_COPY, &debug_info);
        if (rtn != 0) return 0;
        return size;
    }

    Status InvalidInstrCache(const lldb::addr_t &addr,
                             const InterruptPosInfo &pos_info,
                             uint8_t redirect_ifu = 0) const override {
        return Status();
    }

    Status GetWarpsInfo(std::vector<WarpInfo> &warps_info,
                        const InterruptPosInfo &pos_info) const override {
      warps_info = m_warps_info;
      return m_warps_status;
    }

    Status SingleStep(const InterruptPosInfo &pos_info) const override {
      m_single_step_calls.push_back(pos_info);
      return m_single_step_status;
    }

    std::vector<WarpInfo> m_warps_info;
    Status m_warps_status;
    mutable std::vector<InterruptPosInfo> m_single_step_calls;
    Status m_single_step_status;
};

class AscendProcessLinuxTest : public testing::Test {
public:
    virtual void SetUp() {
        fake_system_func.reset(new FakeSystemCallFunc());
        EXPECT_CALL(*fake_system_func, open(_, _)).WillRepeatedly(__real_open);
        EXPECT_CALL(*fake_system_func, ioctl(_, _, _)).WillRepeatedly(__real_ioctl);
    }

    virtual void TearDown() {
        fake_system_func.reset();
    }

    static std::unique_ptr<FakeSystemCallFunc> fake_system_func;
};

std::unique_ptr<FakeSystemCallFunc> AscendProcessLinuxTest::fake_system_func;

extern "C"
{
    int __wrap_open(const char* a, int b) {
        if (AscendProcessLinuxTest::fake_system_func) {
            return AscendProcessLinuxTest::fake_system_func->open(a, b);
        } else {
            return __real_open(a, b);
        }
    }

    int __wrap_ioctl(int a, unsigned long int b, DebugInfo &c) {
        if (AscendProcessLinuxTest::fake_system_func) {
            return AscendProcessLinuxTest::fake_system_func->ioctl(a, b, c);
        } else {
            return __real_ioctl(a, b, c);
        }
    }
}

TEST_F(AscendProcessLinuxTest, HandleStubMessage) {
    HostInfo::Initialize();
    MainLoop mainloop;
    NativeProcessLinux::Manager manager(mainloop);
    GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);

    int terminal_fd = 3;
    ::pid_t pid = 11111;
    ArchSpec arch = ArchSpec("hiipu64");
    llvm::ArrayRef<::pid_t> tids;

    auto process = std::make_unique<AscendProcessLinux>(
        pid, terminal_fd, gdb_server,
        arch, manager, tids);

    std::string deviceMsg = "device_id:0;virtual_device_id:0;tgid:0;soc_version:test;";
    Status status = process->m_parser.ParseMessage(deviceMsg);
    ASSERT_TRUE(status.Success() || status.GetError() != 0);
}

int read_trap_opcodes(int a, unsigned long int b, DebugInfo &c) {
    DmaParam *param = (DmaParam*)c.data;
    uint8_t* tmpdata = (uint8_t*)param->host_addr;
    uint8_t trap_code[4] = {0x00, 0x00, 0x80, 0x41};
    for(int i = 0; i < 4; ++i) {
        tmpdata[i] = trap_code[i];
    }
    return 0;
}

TEST_F(AscendProcessLinuxTest, SetBreakpoint) {
    HostInfo::Initialize();
    MainLoop mainloop;
    NativeProcessLinux::Manager manager(mainloop);
    GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);

    int terminal_fd = 3;
    ::pid_t pid = 11111;
    ArchSpec arch = ArchSpec("hiipu64");
    llvm::ArrayRef<::pid_t> tids;

    auto process = std::make_unique<AscendProcessLinux>(
        pid, terminal_fd, gdb_server,
        arch, manager, tids);

    std::string deviceMsg = "device_id:0;virtual_device_id:0;tgid:0;soc_version:test;";
    process->m_parser.ParseMessage(deviceMsg);

    std::string kernelMsg = "kernel_name:test;kernel_hash:0;pc_base_addr:47;";
    process->m_parser.ParseMessage(kernelMsg);

    process->m_device_context = std::make_shared<FakeDeviceContext>();

    EXPECT_CALL(*fake_system_func, ioctl(_, CMD_GM_COPY, _))
                .WillOnce(Return(0)).WillOnce(Return(0))
                .WillOnce(read_trap_opcodes);
    EXPECT_CALL(*fake_system_func, ioctl(_, CMD_SQ_SEND, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(*fake_system_func, ioctl(_, CMD_CQ_RECV, _)).WillRepeatedly(Return(0));

    lldb::addr_t addr = 0x47;
    uint32_t size = 8;
    EXPECT_THAT_ERROR(process->SetBreakpoint(addr, size, llvm::Triple::ArchType::hiipu64, false).
                        ToError(), llvm::Succeeded());
}

TEST(InterruptPosInfoTest, IsFirstStopPosition_FirstStop_ReturnsTrue) {
  InterruptPosInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 1;
  info.first_stop_core_type = CoreType::AIV;
  info.first_stop_core_id = 1;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  info.thread_pos = {1, 2, 3};
  info.first_stop_thread_pos = {1, 2, 3};
  EXPECT_TRUE(info.IsFirstStopPosition());
}

TEST(InterruptPosInfoTest, IsFirstStopPosition_DifferentCore_ReturnsFalse) {
  InterruptPosInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 1;
  info.first_stop_core_type = CoreType::AIV;
  info.first_stop_core_id = 2;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  EXPECT_FALSE(info.IsFirstStopPosition());
}

TEST(InterruptPosInfoTest, IsFirstStopPosition_DifferentCoreType_ReturnsFalse) {
  InterruptPosInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 1;
  info.first_stop_core_type = CoreType::AIC;
  info.first_stop_core_id = 1;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  EXPECT_FALSE(info.IsFirstStopPosition());
}

TEST(InterruptPosInfoTest,
     IsFirstStopPosition_SimtDifferentThread_ReturnsFalse) {
  InterruptPosInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 1;
  info.first_stop_core_type = CoreType::AIV;
  info.first_stop_core_id = 1;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  info.thread_pos = {1, 2, 3};
  info.first_stop_thread_pos = {4, 5, 6};
  EXPECT_FALSE(info.IsFirstStopPosition());
}

TEST(InterruptPosInfoTest, IsFirstStopPosition_SimtSameThread_ReturnsTrue) {
  InterruptPosInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 1;
  info.first_stop_core_type = CoreType::AIV;
  info.first_stop_core_id = 1;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  info.thread_pos = {1, 2, 3};
  info.first_stop_thread_pos = {1, 2, 3};
  EXPECT_TRUE(info.IsFirstStopPosition());
}

TEST(InterruptPosInfoTest, InvalidPc_EqualsUint64Max) {
  EXPECT_EQ(InterruptPosInfo::INVALID_PC, UINT64_MAX);
}

TEST(InterruptPosInfoTest, GetWarpId_ThreadId5_ReturnsWarp0) {
  InterruptPosInfo info;
  info.thread_info.thread_id = 5;
  EXPECT_EQ(info.GetWarpId(), 0u);
}

TEST(InterruptPosInfoTest, GetWarpId_ThreadId32_ReturnsWarp1) {
  InterruptPosInfo info;
  info.thread_info.thread_id = 32;
  EXPECT_EQ(info.GetWarpId(), 1u);
}

namespace {

std::unique_ptr<AscendProcessLinux>
CreateProcess(MainLoop &mainloop, NativeProcessLinux::Manager &manager,
              GDBRemoteCommunicationServerLLGS &gdb_server,
              std::shared_ptr<FakeDeviceContext> &device_ctx) {
  int terminal_fd = 3;
  ::pid_t pid = 11111;
  ArchSpec arch = ArchSpec("hiipu64");
  llvm::ArrayRef<::pid_t> tids;
  auto process = std::make_unique<AscendProcessLinux>(
      pid, terminal_fd, gdb_server, arch, manager, tids);
  device_ctx = std::make_shared<FakeDeviceContext>();
  process->m_device_context = device_ctx;
  return process;
}

} // namespace

TEST_F(AscendProcessLinuxTest, FixSimtPC_SimtCoreMatchingWarp_UpdatesPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  WarpInfo warp{};
  warp.warp_id = 0;
  warp.simt_pc = 0x1000;
  device_ctx->m_warps_info = {warp};

  process->m_pos_info.thread_info.thread_id = 5; // warp_id = 0

  CoreInfo core_info{};
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  core_info.pc = 0;

  process->FixSimtPC(core_info);

  EXPECT_EQ(core_info.pc, 0x1000ULL);
}

TEST_F(AscendProcessLinuxTest, FixSimtPC_NonSimtCore_DoesNotUpdatePC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  CoreInfo core_info{};
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  core_info.pc = 0x200;

  process->FixSimtPC(core_info);

  EXPECT_EQ(core_info.pc, 0x200ULL);
}

TEST_F(AscendProcessLinuxTest, FixSimtPC_GetWarpsInfoFailed_DoesNotUpdatePC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  device_ctx->m_warps_status = Status("query warps info failed");

  CoreInfo core_info{};
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  core_info.pc = 0x200;

  process->FixSimtPC(core_info);

  EXPECT_EQ(core_info.pc, 0x200ULL);
}

TEST_F(AscendProcessLinuxTest, FixSimtPC_NoMatchingWarp_DoesNotUpdatePC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  WarpInfo warp{};
  warp.warp_id = 1;
  warp.simt_pc = 0x3000;
  device_ctx->m_warps_info = {warp};

  process->m_pos_info.thread_info.thread_id = 5; // warp_id = 0

  CoreInfo core_info{};
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  core_info.pc = 0x200;

  process->FixSimtPC(core_info);

  EXPECT_EQ(core_info.pc, 0x200ULL);
}

TEST_F(AscendProcessLinuxTest, GetStoppedCorePC_NonSimtCore_ReturnsPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  CoreInfo core_info{};
  core_info.core_id = 0;
  core_info.core_type = CoreType::AIV;
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  core_info.pc = 0x200;
  process->m_cores_info = {core_info};

  process->m_pos_info.core_id = 0;
  process->m_pos_info.core_type = CoreType::AIV;

  lldb::addr_t pc = 0;
  Status error = process->GetStoppedCorePC(pc);

  EXPECT_TRUE(error.Success());
  EXPECT_EQ(pc, 0x200ULL);
}

TEST_F(AscendProcessLinuxTest, GetStoppedCorePC_SimtCore_ReturnsUpdatedPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  WarpInfo warp{};
  warp.warp_id = 0;
  warp.simt_pc = 0x1000;
  device_ctx->m_warps_info = {warp};

  CoreInfo core_info{};
  core_info.core_id = 0;
  core_info.core_type = CoreType::AIV;
  core_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  core_info.pc = 0;
  process->m_cores_info = {core_info};

  process->m_pos_info.core_id = 0;
  process->m_pos_info.core_type = CoreType::AIV;
  process->m_pos_info.thread_info.thread_id = 5; // warp_id = 0

  lldb::addr_t pc = 0;
  Status error = process->GetStoppedCorePC(pc);

  EXPECT_TRUE(error.Success());
  EXPECT_EQ(pc, 0x1000ULL);
}

TEST_F(AscendProcessLinuxTest, GetStoppedCorePC_NoMatchingCore_Fails) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  CoreInfo core_info{};
  core_info.core_id = 0;
  core_info.core_type = CoreType::AIC;
  core_info.pc = 0x200;
  process->m_cores_info = {core_info};

  process->m_pos_info.core_id = 5;
  process->m_pos_info.core_type = CoreType::AIV;

  lldb::addr_t pc = 0;
  Status error = process->GetStoppedCorePC(pc);

  EXPECT_TRUE(error.Fail());
}

namespace {

WarpInfo MakeWarp(uint8_t warp_id, uint64_t simt_pc, uint32_t exec_mask) {
  WarpInfo warp{};
  warp.core_id = 0;
  warp.warp_id = warp_id;
  warp.warp_num = 2;
  warp.simt_pc = simt_pc;
  warp.exec_mask = exec_mask;
  return warp;
}

InterruptEvent MakeSimtBreakpointEvent(uint64_t pc, uint16_t thread_id) {
  InterruptEvent event{};
  event.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  event.status = CoreStatus::BRKPT;
  event.core_type = static_cast<uint8_t>(CoreType::AIV);
  event.core_id = 0;
  event.pc = pc;
  event.thread_info.thread_dim_x = 32;
  event.thread_info.thread_dim_y = 1;
  event.thread_info.thread_dim_z = 1;
  event.thread_info.thread_id = thread_id;
  return event;
}

// 通过公共只读接口拿到 map 后 const_cast 写入，避免依赖 private hack。
void AddHardwareBreakpoint(AscendProcessLinux *process, lldb::addr_t addr,
                           size_t size) {
  const_cast<HardwareBreakpointMap &>(
      process->GetHardwareBreakpointMap())[addr] = {addr, size};
}

CoreInfo MakeSimtCore(uint8_t core_id, CoreType core_type, uint64_t pc) {
  CoreInfo core{};
  core.core_id = core_id;
  core.core_type = core_type;
  core.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  core.pc = pc;
  return core;
}

} // namespace

// ---- AscendDisassembler950::IsBarThreadBlock ----

TEST(AscendDisassembler950Test, IsBarThreadBlock_Match) {
  EXPECT_TRUE(AscendDisassembler950::IsBarThreadBlock(0x00AE001C));
}

TEST(AscendDisassembler950Test, IsBarThreadBlock_NonMaskBitsAny_Match) {
  // 非掩码位（bit30）任意，仍应命中。
  EXPECT_TRUE(AscendDisassembler950::IsBarThreadBlock(0x40AE001C));
}

TEST(AscendDisassembler950Test, IsBarThreadBlock_Bit0Set_NoMatch) {
  EXPECT_FALSE(AscendDisassembler950::IsBarThreadBlock(0x00AE001D));
}

TEST(AscendDisassembler950Test, IsBarThreadBlock_Bits5To2Wrong_NoMatch) {
  EXPECT_FALSE(AscendDisassembler950::IsBarThreadBlock(0x00AE0018));
}

TEST(AscendDisassembler950Test, IsBarThreadBlock_HighFieldWrong_NoMatch) {
  EXPECT_FALSE(AscendDisassembler950::IsBarThreadBlock(0x00AC001C));
}

// ---- AscendProcessLinux::FixSimtFocus ----

TEST_F(AscendProcessLinuxTest, FixSimtFocus_ReportedPcNotHwBp_FocusHitWarp) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  AddHardwareBreakpoint(process.get(), 0x1000, 4);
  // 命中 warp 的 exec_mask = 0b100，第一个 active lane = 2。
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 0b100)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x400, 5);
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x1000ULL);
  EXPECT_EQ(event.thread_info.thread_id, 2u);
  EXPECT_EQ(process->m_pos_info.pc, 0x1000ULL);
  EXPECT_EQ(process->m_pos_info.thread_info.thread_id, 2u);
  EXPECT_EQ(process->m_pos_info.thread_pos.x, 2u);
}

TEST_F(AscendProcessLinuxTest, FixSimtFocus_ReportedPcIsHwBp_NoChange) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  AddHardwareBreakpoint(process.get(), 0x1000, 4);
  device_ctx->m_warps_info = {MakeWarp(1, 0x1000, 1)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x1000, 5);
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x1000ULL);
  EXPECT_EQ(event.thread_info.thread_id, 5u);
}

TEST_F(AscendProcessLinuxTest, FixSimtFocus_NoMatchingWarp_NoChange) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  AddHardwareBreakpoint(process.get(), 0x1000, 4);
  device_ctx->m_warps_info = {MakeWarp(0, 0x2000, 1)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x400, 5);
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x400ULL);
  EXPECT_EQ(event.thread_info.thread_id, 5u);
}

TEST_F(AscendProcessLinuxTest, FixSimtFocus_SingleStepEvent_NoChange) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  AddHardwareBreakpoint(process.get(), 0x1000, 4);
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x400, 5);
  event.status = CoreStatus::SINGLE_STEP;
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x400ULL);
  EXPECT_EQ(event.thread_info.thread_id, 5u);
}

TEST_F(AscendProcessLinuxTest, FixSimtFocus_NonSimt_NoChange) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  AddHardwareBreakpoint(process.get(), 0x1000, 4);
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x400, 5);
  event.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x400ULL);
  EXPECT_EQ(event.thread_info.thread_id, 5u);
}

TEST_F(AscendProcessLinuxTest, FixSimtFocus_EmptyHwBp_NoChange) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1)};

  InterruptEvent event = MakeSimtBreakpointEvent(0x400, 5);
  process->FixSimtFocus(event);

  EXPECT_EQ(event.pc, 0x400ULL);
  EXPECT_EQ(event.thread_info.thread_id, 5u);
}

// ---- 切核/切线程后刷新 m_pos_info.pc ----

TEST_F(AscendProcessLinuxTest, SetAivOnFocus_RefreshesFocusedPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1)};
  process->m_cores_info = {MakeSimtCore(0, CoreType::AIV, 0)};
  process->m_pos_info.thread_info.thread_id = 5; // warp_id = 0

  process->SetAivOnFocus(0);

  EXPECT_EQ(process->m_pos_info.pc, 0x1000ULL);
}

TEST_F(AscendProcessLinuxTest, SetThreadOnFocus_RefreshesFocusedPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1), MakeWarp(1, 0x2000, 1)};
  process->m_cores_info = {MakeSimtCore(0, CoreType::AIV, 0)};
  process->m_pos_info.core_id = 0;
  process->m_pos_info.core_type = CoreType::AIV;
  process->m_pos_info.thread_info.thread_dim_x = 32;
  process->m_pos_info.thread_info.thread_dim_y = 2;
  process->m_pos_info.thread_info.thread_dim_z = 1;

  process->SetThreadOnFocus(40); // thread 40 -> warp 1

  EXPECT_EQ(process->m_pos_info.pc, 0x2000ULL);
}

TEST_F(AscendProcessLinuxTest, SetAivOnFocus_NoMatchingCore_KeepsPC) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  process->m_cores_info = {MakeSimtCore(0, CoreType::AIV, 0)};
  process->m_pos_info.pc = 0xAA;

  process->SetAivOnFocus(9); // 无匹配 core

  EXPECT_EQ(process->m_pos_info.pc, 0xAAULL);
}

// ---- AscendProcessLinux::SingleStep 的 SIMT warp 选择 ----

TEST_F(AscendProcessLinuxTest, SingleStep_NonSimt_DelegatesOnce) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  process->m_pos_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;

  process->SingleStep();

  EXPECT_EQ(device_ctx->m_single_step_calls.size(), 1u);
}

TEST_F(AscendProcessLinuxTest, SingleStep_SimtAllWarpAtPc_OneAllWarpCommand) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  process->m_pos_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  process->m_pos_info.pc = 0x1000;
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1), MakeWarp(1, 0x1000, 1)};

  process->SingleStep();

  ASSERT_EQ(device_ctx->m_single_step_calls.size(), 1u);
  EXPECT_FALSE(device_ctx->m_single_step_calls[0].single_warp_run);
  EXPECT_EQ(device_ctx->m_single_step_calls[0].pc, 0x1000ULL);
}

TEST_F(AscendProcessLinuxTest, SingleStep_SimtOneWarpAtPc_StepsThatWarp) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  process->m_pos_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  process->m_pos_info.pc = 0x1000;
  process->m_pos_info.thread_info.thread_dim_x = 32;
  process->m_pos_info.thread_info.thread_dim_y = 1;
  process->m_pos_info.thread_info.thread_dim_z = 1;
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1), MakeWarp(1, 0x2000, 1)};

  process->SingleStep();

  ASSERT_EQ(device_ctx->m_single_step_calls.size(), 1u);
  EXPECT_TRUE(device_ctx->m_single_step_calls[0].single_warp_run);
  EXPECT_EQ(device_ctx->m_single_step_calls[0].thread_pos.x, 0u);
}

TEST_F(AscendProcessLinuxTest,
       SingleStep_SimtNoWarpAtPc_FallbackInterruptWarp) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  process->m_pos_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  process->m_pos_info.pc = 0x3000;
  process->m_pos_info.thread_info.thread_id = 5; // warp_id = 0
  process->m_pos_info.thread_info.thread_dim_x = 32;
  process->m_pos_info.thread_info.thread_dim_y = 1;
  process->m_pos_info.thread_info.thread_dim_z = 1;
  device_ctx->m_warps_info = {MakeWarp(0, 0x1000, 1), MakeWarp(1, 0x2000, 1)};

  process->SingleStep();

  ASSERT_EQ(device_ctx->m_single_step_calls.size(), 1u);
  EXPECT_TRUE(device_ctx->m_single_step_calls[0].single_warp_run);
  EXPECT_EQ(device_ctx->m_single_step_calls[0].thread_pos.x, 0u);
}

TEST_F(AscendProcessLinuxTest, SingleStep_GetWarpsInfoFailed_Fallback) {
  HostInfo::Initialize();
  MainLoop mainloop;
  NativeProcessLinux::Manager manager(mainloop);
  GDBRemoteCommunicationServerLLGS gdb_server(mainloop, manager);
  std::shared_ptr<FakeDeviceContext> device_ctx;
  auto process = CreateProcess(mainloop, manager, gdb_server, device_ctx);

  device_ctx->m_warps_status = Status("query warps info failed");
  process->m_pos_info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;

  process->SingleStep();

  EXPECT_EQ(device_ctx->m_single_step_calls.size(), 1u);
}

#endif
