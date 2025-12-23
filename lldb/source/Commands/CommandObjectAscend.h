/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef LLDB_SOURCE_COMMANDS_COMMANDOBJECTASCEND_H
#define LLDB_SOURCE_COMMANDS_COMMANDOBJECTASCEND_H

#include "lldb/Interpreter/CommandObjectMultiword.h"

namespace lldb_private {

class CommandObjectMultiwordAscend : public CommandObjectMultiword {
public:
  CommandObjectMultiwordAscend(CommandInterpreter &interpreter);

  ~CommandObjectMultiwordAscend() override;
};

} // namespace lldb_private

#endif // LLDB_SOURCE_COMMANDS_COMMANDOBJECTASCEND_H
#endif