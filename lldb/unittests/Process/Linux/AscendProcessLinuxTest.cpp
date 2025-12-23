/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "lldb/Host/Host.h"
#include "lldb/Host/HostInfo.h"
#include "Plugins/Process/Linux/AscendProcessLinux.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "Plugins/Process/Utility/LinuxProcMaps.h"
#include "Plugins/Process/gdb-remote/GDBRemoteCommunicationServerLLGS.h"
#include "Plugins/Process/gdb-remote/ProcessGDBRemoteLog.h"
#define private public  // hack complier
#define protected public
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

typedef process_linux::NativeProcessLinux::Factory NativeProcessFactory;

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
    NativeProcessFactory factory;
    GDBRemoteCommunicationServerLLGS gdb_server(mainloop, factory);

    int terminal_fd = 3;
    ::pid_t pid = 11111;
    ArchSpec arch = ArchSpec("hiipu64");
    int sockets[2]; /* the pair of socket descriptors */
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    ASSERT_TRUE(ret != -1);
    int our_socket = sockets[0];
    int gdb_socket = sockets[1];
    llvm::ArrayRef<::pid_t> tids({pid});

    auto process = std::make_unique<AscendProcessLinux>(
        pid, terminal_fd, gdb_server,
        arch, mainloop, tids, our_socket);
    std::string resp = "$ok;";
    std::string deviceMsg = "$device_id:0;";

    ret = write(gdb_socket, deviceMsg.c_str(), deviceMsg.length());
    ASSERT_TRUE(ret >= (int)deviceMsg.length());
    process->HandleStubMessage();
    char buf[30] = {0};
    ret = read(gdb_socket, buf, sizeof(buf));
    ASSERT_TRUE(ret >= 0);
    printf("read bytes=%s\n", buf);
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
    NativeProcessFactory factory;
    GDBRemoteCommunicationServerLLGS gdb_server(mainloop, factory);

    int terminal_fd = 3;
    ::pid_t pid = 11111;
    ArchSpec arch = ArchSpec("hiipu64");
    int sockets[2]; /* the pair of socket descriptors */
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    ASSERT_TRUE(ret != -1);
    int our_socket = sockets[0];
    int gdb_socket = sockets[1];
    llvm::ArrayRef<::pid_t> tids({pid});

    auto process = std::make_unique<AscendProcessLinux>(
        pid, terminal_fd, gdb_server,
        arch, mainloop, tids, our_socket);
 
    std::string resp = "$ok;";
    std::string deviceMsg = "$device_id:0;";

    ret = write(gdb_socket, deviceMsg.c_str(), deviceMsg.length());
    ASSERT_TRUE(ret >= (int)deviceMsg.length());
    process->HandleStubMessage();
    char buf[30] = {0};
    ret = read(gdb_socket, buf, sizeof(buf));
    ASSERT_TRUE(ret >= 0);

    std::string pcMsg = "$pc_base_addr:47;";
    ret = write(gdb_socket, pcMsg.c_str(), pcMsg.length());
    ASSERT_TRUE(ret >= (int)pcMsg.length());
    process->HandleStubMessage();
    ret = read(gdb_socket, buf, sizeof(buf));
    ASSERT_TRUE(ret >= 0);
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
