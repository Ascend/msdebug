/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef ASCENDCOMMUNICATIONCLIENT_H
#define ASCENDCOMMUNICATIONCLIENT_H
#include "lldb/Host/Socket.h"
#include <mutex>
#include <string>

class AscendCommunicationClient {
public:
  explicit AscendCommunicationClient();
  static AscendCommunicationClient &GetInstance(std::int32_t deviceId);
  int SendAndWaitResponse(std::string const &sendData, std::string& resp);
private:
  int Write(std::string const &msg);
  int Read(std::string& msg) const;
  static std::mutex device_mutex_;
  std::unique_ptr<lldb_private::Socket> domain_client_;
  std::mutex write_mutex_;
  std::mutex mtx_;
};

#endif //ASCENDCOMMUNICATIONCLIENT_H
#endif
