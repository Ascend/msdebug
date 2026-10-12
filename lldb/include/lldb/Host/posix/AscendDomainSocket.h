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
