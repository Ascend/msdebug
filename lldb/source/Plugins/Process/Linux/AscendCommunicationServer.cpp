/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "AscendCommunicationServer.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Host/posix/AscendDomainSocket.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/StringExtractorGDBRemote.h"
#include "llvm/Support/SHA256.h"

#include <fstream>
#include <iomanip>
#include <regex>
#include <unistd.h>

using namespace llvm;
using namespace lldb_private;
using namespace std;

enum MESSAGE_ERROR_CODE {
  INVALID_DEVICE_INFO_ERR = 0x20205,
  INVALID_KERNEL_INFO_ERR = 0x20206,
  INVALID_STREAM_ID_ERR = 0x20207,
  INVALID_HEADER_ERR = 0x20208,
  INVALID_IPC_MEM_INFO_ERR = 0x20209
};

AscendCommunicationServer::AscendCommunicationServer(std::size_t max_client_num,
  const std::string& socket_path)
    : m_socket_path(socket_path), m_max_client_num(max_client_num), m_is_running(true) {
  m_domain_socket = std::make_unique<AscendDomainSocket>(true, false);
}

AscendCommunicationServer::~AscendCommunicationServer() {
  Close();
}

void AscendCommunicationServer::Close() {
  m_is_running = false;
  if (m_accept_worker.joinable()) {
    m_accept_worker.join();
  }
  for (auto &th : m_read_worker) {
    if (th.joinable()) {
      th.join();
    }
  }
  if (!m_socket_path.empty()) {
    if (unlink(m_socket_path.c_str()) != 0) {
      LLDB_LOG(GetLog(POSIXLog::Process), "Failed to delete msdebug socket file");
    }
    m_socket_path = "";
  }
}

bool AscendCommunicationServer::Start() {
  Log *log = GetLog(POSIXLog::Process);
  if (m_socket_path.empty() || !m_domain_socket) {
    LLDB_LOG(log, "m_socket_path is empty or m_domain_socket is nullptr");
    return false;
  }
  Status error = m_domain_socket->Listen(m_socket_path, m_max_client_num);
  if (error.Success()) {
    LLDB_LOG(log, "Listen success, m_socket_path={0}", m_socket_path);
  } else {
    LLDB_LOG(log, "Listen falied, {0}", error);
  }
  m_accept_worker = std::thread([this]() {
    Log *log = GetLog(POSIXLog::Thread);
    while (m_is_running) {
      Socket *client_socket;
      Status error = m_domain_socket->Accept(client_socket);
      if (error.Fail()) {
        if (std::string(error.AsCString())
            .find("Resource temporarily unavailable") == std::string::npos) {
          LLDB_LOG(log, "Accept falied, {0}", error);
        }
        continue;
      }
      LLDB_LOG(log, "Accept success");
      if (m_msg_handler_hook != nullptr) {
        std::thread th = std::thread(&AscendCommunicationServer::Listen, this, client_socket);
        m_read_worker.emplace_back(std::move(th));
      } else {
        LLDB_LOG(log, "m_msg_handler_hook = nullptr");
      }
    }
  });
  return true;
}

class PacketExtractor {
public:
  void Feed(const void *src, size_t len);

  bool ExtractEscapedPacket(std::string &packet) const;

  void Clear();
private:
  std::string m_bytes;
  size_t m_end_pos;
};

void PacketExtractor::Feed(const void *src, size_t len) {
  m_bytes.append(static_cast<const char *>(src), len);
  m_end_pos = m_bytes.find('#', m_bytes.length() - len);
}

bool PacketExtractor::ExtractEscapedPacket(string &packet) const {
  packet.clear();
  if (m_end_pos >= m_bytes.size()) {
    return false;
  }
  // Skip the dollar sign, and last '#'
  for (size_t i = 1; i < m_end_pos; i++) {
    const char ch = m_bytes[i];
    if (ch == 0x7d) {
      const char escapee = m_bytes[++i] ^ 0x20;
      packet.push_back(escapee);
    } else {
      packet.push_back(ch);
    }
  }
  return true;
}

void PacketExtractor::Clear() {
  m_bytes.clear();
  m_end_pos = std::string::npos;
}

void AscendCommunicationServer::Listen(Socket *client_socket) {
  if (!client_socket) {
    return;
  }
  std::string packet;
  PacketExtractor extractor;
  static constexpr std::size_t MAX_SIZE = 8192;
  std::vector<char> buffer(MAX_SIZE);
  size_t read_size;
  while (m_is_running) {
    while(client_socket->IsValid()) {
      read_size = MAX_SIZE;
      Status status = client_socket->Read(buffer.data(), read_size);
      if (read_size > 0) {
        extractor.Feed(buffer.data(), read_size);
        if (extractor.ExtractEscapedPacket(packet)) {
          break;
        }
      }
    }
    extractor.Clear();
    m_msg_handler_hook(client_socket, packet);
  }
}

void AscendCommunicationServer::SetMsgHandlerHook(ClientMsgHandlerHook &&hook) {
  m_msg_handler_hook = hook;
}

HandleResult DeviceHandler::Parse(const std::string& msg) {
  Status error;
  std::smatch matches;
  if (std::regex_search(msg, matches, std::regex("device_id:(\\d+);tgid:(\\d+);soc_version:([^;]+);"))) {
    m_device_info.device_id = std::stoi(matches[1]);
    m_device_info.tgid = std::stoi(matches[2]);
    m_device_info.soc_version = matches[3];
  } else {
    error.SetError(INVALID_DEVICE_INFO_ERR, lldb::eErrorTypeGeneric);
  }
  return error;
}

static void ShowKernelHashReceived(const void *data, size_t num_bytes) {
  Log *log = GetLog(POSIXLog::Process);
  llvm::SHA256 hasher;
  llvm::ArrayRef<uint8_t> array_data(static_cast<const uint8_t*>(data), num_bytes);
  std::array<uint8_t, 32> result = hasher.hash(array_data);
  std::stringstream hash_ss;
  for (const uint8_t byte : result) {
    hash_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte);
  }
  LLDB_LOG(log, "Got device binary {0} bytes from remote, hash={1}", num_bytes, hash_ss.str());
}

HandleResult KernelHandler::Parse(const std::string& msg) {
  Log *log = GetLog(POSIXLog::Process);
  Status error;
  StringExtractorGDBRemote packet = StringExtractorGDBRemote(msg);
  StringRef key;
  StringRef value;
  m_kernel_info = KernelInfoMsg();
  // binary can not use key value to parse, binary may contains random char
  constexpr int max_colon = 4;
  int num_colon = 0;
  // we should not change index after max_colon times get from packet
  while (num_colon < max_colon && packet.GetNameColonValue(key, value) ) {
    if (key.compare("kernel_name") == 0) {
      m_kernel_info.kernel_name = value;
    } else if (key.compare("kernel_hash") == 0) {
      m_kernel_info.kernel_hash = value;
    } else if (key.compare("pc_base_addr") == 0) {
      value.getAsInteger(16, m_kernel_info.pc_base_addr);
      LLDB_LOGF(log, "m_kernel_info.pc_base_addr=0x%lx", m_kernel_info.pc_base_addr);
    } else if (key.compare("stream_id") == 0) {
      value.getAsInteger(10, m_kernel_info.stream_id);
      LLDB_LOG(log, "m_kernel_info.stream_id={0}", m_kernel_info.stream_id);
    }
    num_colon++;
  }
  // only kernel name and stream_id updated
  if (num_colon == 2) {
    return error;
  }
  if (num_colon != max_colon) {
    error.SetError(INVALID_KERNEL_INFO_ERR, lldb::eErrorTypeGeneric);
    return error;
  }
  if (!packet.Consume("kernel_binary:")) {
    LLDB_LOG(log, "packet is invalid: {0}", packet.Peek());
    error.SetError(INVALID_KERNEL_INFO_ERR, lldb::eErrorTypeGeneric);
    return error;
  }
  size_t num_bytes = packet.GetBytesLeft();
  // elf binary must large than 10
  constexpr size_t size_elf_header = 50;
  if (num_bytes < size_elf_header) {
    LLDB_LOG(log, "packet remain {0} bytes, is too less than {1}", num_bytes, size_elf_header);
    error.SetError(INVALID_KERNEL_INFO_ERR, lldb::eErrorTypeGeneric);
    return error;
  }
  // last is ';'
  num_bytes -= 1;
  const char *data = packet.Peek();
  if (data == nullptr) {
    error.SetError(INVALID_KERNEL_INFO_ERR, lldb::eErrorTypeGeneric);
    return error;
  }
  m_kernel_info.elf.assign(data, data + num_bytes);
  ShowKernelHashReceived(data, num_bytes);
  return error;
}

HandleResult IpcMemHandler::Parse(const std::string& msg) {
  Log *log = GetLog(POSIXLog::Process);
  Status error;
  // ipc_mem msg pattern:
  // "$ipc_mem_addr:0x1240000000;size:9000;key:xxxxxxxxxxxxxxxxxxxxxxxx#"
  // "$ipc_mem_addr:0x1240000000;free:1#"

  StringExtractorGDBRemote packet = StringExtractorGDBRemote(msg);
  StringRef key;
  StringRef value;
  m_ipc_mem_info = IpcMemInfoMsg();
  constexpr int max_colon = 3;
  int num_colon = 0;

  // we should not change index after max_colon times get from packet
  while (num_colon < max_colon && packet.GetNameColonValue(key, value) ) {
    if (key.compare("ipc_mem_addr") == 0) {
      value.getAsInteger(16, m_ipc_mem_info.addr);
      LLDB_LOGF(log, "m_ipc_mem_info.addr=0x%lx", m_ipc_mem_info.addr);
    } else if (key.compare("size") == 0) {
      value.getAsInteger(10, m_ipc_mem_info.size);
      LLDB_LOGF(log, "m_ipc_mem_info.size=%lu", m_ipc_mem_info.size);
    } else if (key.compare("free") == 0) {
      value.getAsInteger(10, m_ipc_mem_info.free);
      LLDB_LOGF(log, "m_ipc_mem_info.size=%d", m_ipc_mem_info.free);
    } else if (key.compare("key") == 0) {
      std::string tempVal(value);
      tempVal.copy(m_ipc_mem_info.key, IpcMemInfoMsg::KEY_LEN);
      LLDB_LOGF(log, "tempVal=%s", tempVal.c_str());
      LLDB_LOGF(log, "m_ipc_mem_info.key=%s", m_ipc_mem_info.key);
    }
    num_colon++;
  }

  if (num_colon > max_colon) {
    error.SetError(INVALID_IPC_MEM_INFO_ERR, lldb::eErrorTypeGeneric);
    return error;
  }
  return error;
}

void MsgParser::Register(const std::string& prefix, std::shared_ptr<MsgHandler> handler) {
  m_handlers[prefix] = {
    handler
  };
}

HandleResult MsgParser::ParseMessage(const std::string& msg) const {
  for (const auto& pair: m_handlers) {
    const auto &prefix = pair.first;
    const auto &entry = pair.second;
    if (msg.compare(0, prefix.size(), prefix) == 0) {
      Status parse_status = entry.handler->Parse(msg);
      if (!parse_status.Success())
        return parse_status;
      return entry.handler->Handle();
    }
  }
  Status error;
  error.SetError(INVALID_HEADER_ERR, lldb::eErrorTypeGeneric);
  return error;
}

#endif
