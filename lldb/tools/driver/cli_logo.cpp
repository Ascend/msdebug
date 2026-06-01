
//===-- cli_logo.cpp -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifdef MS_DEBUGGER
#include <cstring>
#include <unistd.h>

#include "llvm/Support/raw_ostream.h"

#include "cli_logo.h"

namespace {

constexpr const char *kReset = "\033[0m";
constexpr const char *kDimGray = "\033[38;5;240m";
constexpr const char *kBoldWhite = "\033[1;97m";
constexpr const char *kHighlight = "\033[48;5;21;38;5;46m"; // green on blue

bool ShouldUseColorLogo() {
  if (!isatty(STDERR_FILENO)) {
    return false;
  }
  const char *term = std::getenv("TERM");
  return term && std::strcmp(term, "dumb") != 0 &&
         std::strcmp(term, "unknown") != 0;
}

} // namespace

void PrintLogo() {
  if (!ShouldUseColorLogo()) {
    llvm::errs()
        << "================================================================="
        << "\n"
        << "                   >>>>>   MindStudio   <<<<<" << "\n"
        << "    THE END-TO-END TOOLCHAIN TO UNLEASH HUAWEI ASCEND COMPUTE"
        << "\n"
        << "================================================================="
        << "\n\n";
    return;
  }

  llvm::errs()
      << kDimGray
      << "================================================================="
      << kReset << "\n"
      << kBoldWhite << "                   >>>>>  " << kHighlight
      << " MindStudio " << kReset << kBoldWhite << "  <<<<<" << kReset << "\n"
      << kBoldWhite
      << "    THE END-TO-END TOOLCHAIN TO UNLEASH HUAWEI ASCEND COMPUTE"
      << kReset << "\n"
      << kDimGray
      << "================================================================="
      << kReset << "\n\n";
}

#endif // MS_DEBUGGER
