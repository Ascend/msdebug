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
#include "lldb/Host/posix/AscendDomainSocket.h"

#include <unistd.h>
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Utility/Log.h"

using namespace lldb_private;

Status AscendDomainSocket::Listen(llvm::StringRef name, int backlog) {
  Status error = DomainSocket::Listen(name, backlog);
  if (error.Fail()) {
    return error;
  }
  auto timeout = timeval {};
  timeout.tv_sec = 1; // 暂设置read超时时间为1s
  timeout.tv_usec = 0;
  if (setsockopt(GetNativeSocket(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
    error.SetErrorStringWithFormatv("Socket set timeout failed: {0}", std::string(strerror(errno)).c_str());
    return error;
  }
  constexpr int opt = 1;
  if (setsockopt(GetNativeSocket(), SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt)) == -1) {
    error.SetErrorStringWithFormatv("Socket set SO_PEERCRED failed: {0}", std::string(strerror(errno)));
    return error;
  }
  return error;
}

Status AscendDomainSocket::Accept(Socket *&socket) {
  Status error = DomainSocket::Accept(socket);
  if (error.Fail()) {
    return error;
  }
  // 获取客户端凭证（uid/gid）
  struct ucred cred{};
  socklen_t cred_len = sizeof(cred);
  if (getsockopt(socket->GetNativeSocket(), SOL_SOCKET, SO_PEERCRED, &cred,
                 &cred_len) == -1) {
    error.SetErrorStringWithFormatv("Get client SO_PEERCRED failed: {0}", std::string(strerror(errno)));
    return error;
  }

  if (getuid() != cred.uid) {
    error.SetErrorStringWithFormatv("Client check permission failed, recv id: uid={0}", cred.uid);
    return error;
  }
  return error;
}
#endif
