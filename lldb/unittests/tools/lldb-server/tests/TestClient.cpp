//===-- TestClient.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TestClient.h"
#include "TestingSupport/Host/SocketTestUtilities.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Host/common/TCPSocket.h"
#include "lldb/Host/posix/ConnectionFileDescriptorPosix.h"
#include "lldb/Utility/Args.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <future>
#include <sstream>
#include <string>
#include <sys/socket.h>

using namespace lldb;
using namespace lldb_private;
using namespace llvm;
using namespace llgs_tests;

#ifdef SendMessage
#undef SendMessage
#endif

TestClient::TestClient(std::unique_ptr<Connection> Conn) {
  SetConnection(std::move(Conn));
  SetPacketTimeout(std::chrono::seconds(10));
}

TestClient::~TestClient() {
  if (!IsConnected())
    return;

  EXPECT_THAT_ERROR(SendMessage("k"), Succeeded());
}

Error TestClient::initializeConnection() {
  if (SendAck() == 0)
    return make_error<StringError>("Sending initial ACK failed.",
                                   inconvertibleErrorCode());

  if (Error E = SendMessage("QStartNoAckMode"))
    return E;

  m_send_acks = false;
  return Error::success();
}

Expected<std::unique_ptr<TestClient>> TestClient::launch(StringRef Log) {
  return launch(Log, {});
}

Expected<std::unique_ptr<TestClient>> TestClient::launch(StringRef Log, ArrayRef<StringRef> InferiorArgs) {
  return launchCustom(Log, false, {}, InferiorArgs);
}

Expected<std::unique_ptr<TestClient>>
TestClient::launchCustom(StringRef Log, bool disable_stdio,
                         ArrayRef<StringRef> ServerArgs,
                         ArrayRef<StringRef> InferiorArgs) {
  const ArchSpec &arch_spec = HostInfo::GetArchitecture();
  Args args;
  args.AppendArgument(LLDB_SERVER);
  if (IsLldbServer())
    args.AppendArgument("gdbserver");

  if (!Log.empty()) {
    args.AppendArgument(("--log-file=" + Log).str());
    if (IsLldbServer())
      args.AppendArgument("--log-channels=gdb-remote packets");
    else
      args.AppendArgument("--log-flags=0x800000");
  }

  int sockets[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0)
    return llvm::createStringError(inconvertibleErrorCode(),
                                   "socketpair failed");
  int client_fd = sockets[0];
  int server_fd = sockets[1];

  args.AppendArgument(formatv("--fd={0}", server_fd).str());

  for (StringRef arg : ServerArgs)
    args.AppendArgument(arg);

  ProcessLaunchInfo Info;
  Info.SetArchitecture(arch_spec);
  Info.SetArguments(args, true);
  Info.GetEnvironment() = Host::GetEnvironment();
  Info.SetMonitorProcessCallback(&ProcessLaunchInfo::NoOpMonitorCallback);
  Info.AppendDuplicateFileAction(server_fd, server_fd);

  if (disable_stdio)
    Info.GetFlags().Set(lldb::eLaunchFlagDisableSTDIO);
  Status status = Host::LaunchProcess(Info);
  if (status.Fail()) {
    close(client_fd);
    close(server_fd);
    return status.ToError();
  }

  close(server_fd);

  auto Conn = std::make_unique<ConnectionFileDescriptor>(
      new TCPSocket(client_fd, true, false));
  auto Client = std::unique_ptr<TestClient>(new TestClient(std::move(Conn)));

  if (Error E = Client->initializeConnection())
    return std::move(E);

  if (!InferiorArgs.empty()) {
    std::vector<std::string> args_vec;
    for (const auto &arg : InferiorArgs)
      args_vec.push_back(arg.str());
    if (Error E = Client->SetInferior(args_vec))
      return std::move(E);
  }

  return std::move(Client);
}

Error TestClient::SetInferior(llvm::ArrayRef<std::string> inferior_args) {
  if (SendEnvironment(Host::GetEnvironment()) != 0) {
    return make_error<StringError>("Failed to set launch environment",
                                   inconvertibleErrorCode());
  }
  std::stringstream command;
  command << "A";
  for (size_t i = 0; i < inferior_args.size(); i++) {
    if (i > 0)
      command << ',';
    std::string hex_encoded = toHex(inferior_args[i]);
    command << hex_encoded.size() << ',' << i << ',' << hex_encoded;
  }

  if (Error E = SendMessage(command.str()))
    return E;
  if (Error E = SendMessage("qLaunchSuccess"))
    return E;
  if (Error E = queryProcess())
    return E;
  return Error::success();
}

Error TestClient::ListThreadsInStopReply() {
  return SendMessage("QListThreadsInStopReply");
}

Error TestClient::SetBreakpoint(unsigned long address) {
  return SendMessage(formatv("Z0,{0:x-},1", address).str());
}

Error TestClient::ContinueAll() { return Continue("vCont;c"); }

Error TestClient::ContinueThread(unsigned long thread_id) {
  return Continue(formatv("vCont;c:{0:x-}", thread_id).str());
}

const llgs_tests::ProcessInfo &TestClient::GetProcessInfo() {
  return *m_process_info;
}

Expected<JThreadsInfo> TestClient::GetJThreadsInfo() {
  return SendMessage<JThreadsInfo>("jThreadsInfo", m_register_infos);
}

const StopReply &TestClient::GetLatestStopReply() {
  assert(m_stop_reply);
  return *m_stop_reply;
}

Error TestClient::SendMessage(StringRef message) {
  std::string dummy_string;
  return SendMessage(message, dummy_string);
}

Error TestClient::SendMessage(StringRef message, std::string &response_string) {
  if (Error E = SendMessage(message, response_string, PacketResult::Success))
    return E;
  StringExtractorGDBRemote Extractor(response_string);
  if (Extractor.IsErrorResponse())
    return Extractor.GetStatus().ToError();
  return Error::success();
}

Error TestClient::SendMessage(StringRef message, std::string &response_string,
                              PacketResult expected_result) {
  StringExtractorGDBRemote response;
  GTEST_LOG_(INFO) << "Send Packet: " << message.str();
  PacketResult result = SendPacketAndWaitForResponse(message, response);
  response.GetEscapedBinaryData(response_string);
  GTEST_LOG_(INFO) << "Read Packet: " << response_string;
  if (result != expected_result)
    return make_error<StringError>(
        formatv("Error sending message `{0}`: {1}", message, result).str(),
        inconvertibleErrorCode());

  return Error::success();
}

unsigned int TestClient::GetPcRegisterId() {
  assert(m_pc_register != LLDB_INVALID_REGNUM);
  return m_pc_register;
}

Error TestClient::qProcessInfo() {
  m_process_info = std::nullopt;
  auto InfoOr = SendMessage<ProcessInfo>("qProcessInfo");
  if (!InfoOr)
    return InfoOr.takeError();
  m_process_info = std::move(*InfoOr);
  return Error::success();
}

Error TestClient::qRegisterInfos() {
  uint32_t reg_offset = 0;
  for (unsigned int Reg = 0;; ++Reg) {
    std::string Message = formatv("qRegisterInfo{0:x-}", Reg).str();
    Expected<RegisterInfo> InfoOr = SendMessage<RegisterInfoParser>(Message);
    if (!InfoOr) {
      consumeError(InfoOr.takeError());
      break;
    }
    m_register_infos.emplace_back(std::move(*InfoOr));

    if (m_register_infos[Reg].byte_offset == LLDB_INVALID_INDEX32)
      m_register_infos[Reg].byte_offset = reg_offset;

    reg_offset =
        m_register_infos[Reg].byte_offset + m_register_infos[Reg].byte_size;
    if (m_register_infos[Reg].kinds[eRegisterKindGeneric] ==
        LLDB_REGNUM_GENERIC_PC)
      m_pc_register = Reg;
  }
  if (m_pc_register == LLDB_INVALID_REGNUM)
    return make_parsing_error("qRegisterInfo: generic");
  return Error::success();
}

Error TestClient::queryProcess() {
  if (Error E = qProcessInfo())
    return E;
  if (Error E = qRegisterInfos())
    return E;
  return Error::success();
}

Error TestClient::Continue(StringRef message) {
  assert(m_process_info);

  auto StopReplyOr = SendMessage<StopReply>(
      message, m_process_info->GetEndian(), m_register_infos);
  if (!StopReplyOr)
    return StopReplyOr.takeError();

  m_stop_reply = std::move(*StopReplyOr);
  if (!isa<StopReplyStop>(m_stop_reply)) {
    StringExtractorGDBRemote R;
    PacketResult result = ReadPacket(R, GetPacketTimeout(), false);
    if (result != PacketResult::ErrorDisconnected) {
      return make_error<StringError>(
          formatv("Expected connection close after sending {0}. Got {1}/{2} "
                  "instead.",
                  message, result, R.GetStringRef())
              .str(),
          inconvertibleErrorCode());
    }
  }
  return Error::success();
}
