/*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*/
#ifdef MS_DEBUGGER
#include "get_tiling_key.h"
#include "rt_stub_log.h"
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include "llvm/Support/SHA256.h"
#include <iomanip>
#include <lldb/Utility/AscendVerification.h>
#include <sstream>
#include <vector>

using namespace std;

bool ParseSymbolLine(const llvm::object::ObjectFile &objectFile,
                     const llvm::object::SymbolRef &symbol, KernelInfo &kernelInfo)
{
  auto expectType = symbol.getType();
  auto expectFlags = symbol.getFlags();
  auto expectSection = symbol.getSection();
  auto expectName = symbol.getName();
  auto expectAddress = symbol.getAddress();
  if (expectType && expectFlags && expectSection && expectName) {
    llvm::object::SymbolRef::Type type = std::move(*expectType);
    uint32_t flags = std::move(*expectFlags);
    llvm::object::section_iterator section = std::move(*expectSection);
    llvm::StringRef name = std::move(*expectName);
    bool global = flags & llvm::object::SymbolRef::SF_Global;
    bool weak = flags & llvm::object::SymbolRef::SF_Weak;
    bool absolute = flags & llvm::object::SymbolRef::SF_Absolute;
    char globLoc = ' ';
    if ((section != objectFile.section_end() || absolute) && !weak) {
      globLoc = global ? 'g' : 'l';
    }
    if (globLoc == 'g' && type == llvm::object::SymbolRef::ST_Function && expectSection.get()->isText()) {
      std::string msg;
      if (lldb_private::CheckStringValid(name.data(), msg)) {
        RT_STUB_LOG_WARNING("Invalid character: %s get from binary\n", msg.c_str());
        return false;
      }
      kernelInfo.kernelOffsets.push_back(expectAddress.get());
      kernelInfo.kernelNames.push_back(name.data());
    }
  } else {
    RT_STUB_LOG_WARNING("Get symbol line parameters failed\n");
    return false;
  }
  return true;
}

bool ParseSymbolTable(const llvm::object::ObjectFile &objectFile, KernelInfo &kernelInfo)
{
  for (auto symbol = objectFile.symbol_begin(); symbol != objectFile.symbol_end(); ++symbol) {
    if (!ParseSymbolLine(objectFile, *symbol, kernelInfo)) {
      return false;
    }
  }
  return true;
}

llvm::Expected<llvm::object::OwningBinary<llvm::object::Binary>> CreateOBinary(const uint8_t* data, size_t dataSize)
{
  std::unique_ptr<llvm::MemoryBuffer> buffer =
      llvm::MemoryBuffer::getMemBufferCopy(llvm::StringRef(reinterpret_cast<const char*>(data), dataSize), "buffer");

  llvm::Expected<std::unique_ptr<llvm::object::Binary>> binOrErr =
      llvm::object::createBinary(buffer->getMemBufferRef(), nullptr, true);
  if (!binOrErr) {
    RT_STUB_LOG_ERROR("Create binary by memory buffer failed\n");
    return binOrErr.takeError();
  }
  std::unique_ptr<llvm::object::Binary> &Bin = binOrErr.get();

  return llvm::object::OwningBinary<llvm::object::Binary>(std::move(Bin), std::move(buffer));
}

bool EndsWith(const std::string &fullStr, const std::string &ending)
{
  if (fullStr.length() >= ending.length()) {
    return fullStr.compare(fullStr.length() - ending.length(), ending.length(), ending) == 0;
  }
  return false;
}

string GetSimpleKernelName(const string &fullName)
{
  if (EndsWith(fullName, MIX_AIC_TAIL) ||
      EndsWith(fullName, MIX_AIV_TAIL)) {
    return fullName.substr(0UL, fullName.length() - 8UL);
  }
  return fullName;
}

std::string GetSha256FromKernel(const uint8_t *data, uint64_t len)
{
    llvm::SHA256 hasher;
    llvm::ArrayRef<uint8_t> array(data, len);
    std::array<uint8_t, 32> result = hasher.hash(array);
    std::stringstream ss;
    for (const uint8_t byte : result) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte);
    }
    return ss.str();
}

bool GenKernelInfo(const void* data, size_t dataSize, KernelInfo &kernelInfo)
{
  auto expectOBinary = CreateOBinary(static_cast<const uint8_t *>(data), dataSize);
  if (expectOBinary) {
    llvm::object::OwningBinary<llvm::object::Binary> oBinary = std::move(*expectOBinary);
    llvm::object::Binary &binary = *oBinary.getBinary();
    if (llvm::object::ObjectFile *objectFile = llvm::dyn_cast<llvm::object::ObjectFile>(&binary)) {
      return ParseSymbolTable(*objectFile, kernelInfo);
    }
  }
  // dump info like this:
  // address      symbol   segment   type    size     kernelname
  // 0                1 2 3     4                5
  // 0000000000000000 g F .text 00000000000000a8 Abs_d2db1a80c523e7e59a032c95969880af_high_performance_2147483647
  return false;
}

#endif
