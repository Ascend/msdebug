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
    FakeDeviceContext() : DeviceContext(0, 0) {
        m_drv_fd = 3;
    }

    Status Init() override { return Status(); }
    bool StartListenThread() override { return true; }
    Status EnableDebugMode() override { return Status(); }

    Status ReadRegister(const RegisterInfo *reg_info,
                        const InterruptPosInfo &pos_info,
                        RegisterValue &value) override {
        return Status("Not implemented");
    }
    Status GetRegisterAddr(const llvm::StringRef reg_name,
                           CoreType core_type, uint64_t &addr) override {
        return Status("Not implemented");
    }
    Status GetRegisterList(std::vector<std::string> &reg_list,
                           CoreType core_type) override {
        return Status("Not implemented");
    }
    Status CheckRegisterAddr(CoreType core_type, uint64_t addr) override {
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

    std::string deviceMsg = "device_id:0;tgid:0;soc_version:test;";
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

    std::string deviceMsg = "device_id:0;tgid:0;soc_version:test;";
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

#endif
