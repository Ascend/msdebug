/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*/
#ifdef MS_DEBUGGER
#ifndef ASCEND_DOMAIN_SOCKET_H
#define ASCEND_DOMAIN_SOCKET_H 
#include "lldb/Host/posix/DomainSocket.h"
#include "lldb/Utility/Status.h"

class AscendDomainSocket : public lldb_private::DomainSocket {
public:
  AscendDomainSocket(bool should_close, bool child_processes_inherit) :
      DomainSocket(should_close, child_processes_inherit) {}

  lldb_private::Status Listen(llvm::StringRef name, int backlog) override;

  lldb_private::Status Accept(Socket *&socket) override;

protected:
  // 用于生成处于抽象命名空间的socket文件
  size_t GetNameOffset() const override { return 1; }
};

#endif // ASCEND_DOMAIN_SOCKET_H
#endif // MS_DEBUGGER
