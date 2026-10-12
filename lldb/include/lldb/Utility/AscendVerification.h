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

#ifndef LLVM_ASCENDVERIFICATION_H
#define LLVM_ASCENDVERIFICATION_H

#include <regex>
#include <string>
#include <unordered_map>

namespace lldb_private {

inline bool CheckStringValid(const std::string &input, std::string &msg) {
  // 定义格式控制字符的替换表
  const std::unordered_map<char, std::string> controlCharMap = {
          {'\n',     "\\n"},
          {'\f',     "\\f"},
          {'\r',     "\\r"},
          {'\b',     "\\b"},
          {'\t',     "\\t"},
          {'\v',     "\\v"},
          {'\a',     "\\a"},
          {'\u007F', "\\u007F"}  // 删除字符
  };

  for(auto& it : controlCharMap){
    if(input.find(it.first) != std::string::npos) {
      msg = it.second;
      return true;
    }
  }
  return false;
}

inline bool ContainsNullTerminator(const char *str, size_t maxLength) {
  for (size_t i = 0; i < maxLength; ++i) {
    if (str[i] == '\0') {
      return true;
    }
  }
  return false; // 如果在最大长度内没有找到'\0'，则返回false
}

} // namespace lldb_private
#endif //LLVM_ASCENDVERIFICATION_H
