/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef LLDB_UNITTESTS_TESTINGSUPPORT_PLUGINS_ASCENDPROCESSLINUXTESTUTILS_H
#define LLDB_UNITTESTS_TESTINGSUPPORT_PLUGINS_ASCENDPROCESSLINUXTESTUTILS_H

#include <gmock/gmock.h>

using namespace lldb_private;

#define CMD_MAGIC_WORD 'D'
#define CMD_SQ_SEND _IOR(CMD_MAGIC_WORD, 0, DebugInfo*) // 发送SQ
#define CMD_CQ_RECV _IOWR(CMD_MAGIC_WORD, 1, DebugInfo*) // 读取CQ
#define CMD_GM_COPY _IOR(CMD_MAGIC_WORD, 2, DebugInfo*) // 读写GM
#define CMD_DEV_REGISTER _IOR(CMD_MAGIC_WORD, 3, struct ms_debug_info*) // 注册device id

enum DmaDirection {
    DEVDRV_DMA_HOST_TO_DEVICE = 0,
    DEVDRV_DMA_DEVICE_TO_HOST = 1
};

struct DebugInfo {
  uint32_t dev_id;
  int32_t timeout;
  uint32_t data_len;
  int32_t pid;
  uint8_t data[64]; 
};

struct DmaParam {
  uint64_t host_addr;
  uint64_t device_addr;
  uint64_t size;
  DmaDirection direction;
};

class FakeSystemCallFunc {
public:
    virtual ~FakeSystemCallFunc(){}
    MOCK_METHOD2(open, int(const char*, int));
    MOCK_METHOD3(ioctl, int(int, unsigned long int, DebugInfo &));
    MOCK_METHOD3(read, int(int, char*, size_t));
};

extern "C"
{
    int __real_open(const char*, int);
    int __wrap_open(const char* a, int b);
    int __real_ioctl(int, unsigned long int, DebugInfo &);
    int __wrap_ioctl(int a, unsigned long int b, DebugInfo &c);
}

#endif //LLDB_UNITTESTS_TESTINGSUPPORT_PLUGINS_ASCENDPROCESSLINUXTESTUTILS_H
#endif