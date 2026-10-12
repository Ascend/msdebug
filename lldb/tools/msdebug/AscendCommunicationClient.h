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
#ifndef ASCENDCOMMUNICATIONCLIENT_H
#define ASCENDCOMMUNICATIONCLIENT_H
#include "lldb/Host/Socket.h"
#include <mutex>
#include <string>

class AscendCommunicationClient {
public:
  explicit AscendCommunicationClient();
  static AscendCommunicationClient &GetInstance(std::int32_t deviceId);
  size_t SendAndWaitResponse(std::string const &sendData, std::string& resp);
private:
  size_t Write(std::string const &msg);
  size_t Read(std::string& msg) const;
  static std::mutex device_mutex_;
  std::unique_ptr<lldb_private::Socket> domain_client_;
  std::mutex write_mutex_;
  std::mutex mtx_;
};

#endif //ASCENDCOMMUNICATIONCLIENT_H
#endif
