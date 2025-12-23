/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "AscendCommunicationServer.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "lldb/Host/posix/AscendDomainSocket.h"
#include "lldb/Utility/Log.h"

#include <unistd.h>

using namespace lldb_private;

enum MESSAGE_ERROR_CODE {
  INVALID_DEVICE_INFO_ERR = 0x20205,
  INVALID_KERNEL_INFO_ERR = 0x20206,
  INVALID_STREAM_ID_ERR = 0x20207,
  INVALID_HEADER_ERR = 0x20208
};

AscendCommunicationServer::AscendCommunicationServer(std::size_t max_client_num,
  const std::string& socket_path)
    : m_socket_path(socket_path), m_max_client_num(max_client_num), m_is_running(true) {
  m_domain_socket = std::make_unique<AscendDomainSocket>(true, false);
}

AscendCommunicationServer::~AscendCommunicationServer() {
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

void AscendCommunicationServer::Listen(Socket *client_socket) {
  if (!client_socket) {
    return;
  }
  std::string msg;
  while (m_is_running) {
    static constexpr std::size_t MAX_SIZE = 2048ULL;
    size_t read_size = MAX_SIZE;
    std::vector<char> buffer(MAX_SIZE);
    if (client_socket->Read(buffer.data(), read_size).Fail()) {
      continue;
    }
    if (read_size > 0) {
      msg.assign(buffer.data(), read_size);
      m_msg_handler_hook(client_socket, msg);
    }
  }
}

void AscendCommunicationServer::SetMsgHandlerHook(ClientMsgHandlerHook &&hook) {
  m_msg_handler_hook = hook;
}

Status DeviceHandler::Parse(const std::string& msg) {
  Status error;
  std::smatch matches;
  if (std::regex_search(msg, matches, std::regex("\\$device_id:(\\d+);tgid:(\\d+);soc_version:([^;]+);"))) {
    m_device_info.device_id = std::stoi(matches[1]);
    m_device_info.tgid = std::stoi(matches[2]);
    m_device_info.soc_version = matches[3];
  } else {
    error.SetError(INVALID_DEVICE_INFO_ERR, lldb::eErrorTypeGeneric);
  }
  return error;
}

Status KernelHandler::Parse(const std::string& msg) {
  Status error;
  std::smatch matches;
  if (std::regex_search(msg, matches,
    std::regex("\\$kernel_name:([^;]+);kernel_hash:([^;]+);pc_base_addr:([\\d;]+);"))) {
    m_kernel_info.kernel_name = matches[1];
    m_kernel_info.kernel_hash = matches[2];
    m_kernel_info.pc_base_addr = std::stoul(matches[3]);
  } else {
    error.SetError(INVALID_KERNEL_INFO_ERR, lldb::eErrorTypeGeneric);
  }
  return error;
}

Status StreamHandler::Parse(const std::string& msg) {
  Status error;
  std::smatch matches;
  if (std::regex_search(msg, matches, std::regex("\\$stream_id:(\\d+);"))) {
    m_stream_id = std::stoi(matches[1]);
  } else {
    error.SetError(INVALID_STREAM_ID_ERR, lldb::eErrorTypeGeneric);
  }
  return error;
}

void MsgParser::Register(const std::string& prefix, const std::string& pattern_str,
                                  std::shared_ptr<MsgHandler> handler) {
  std::regex pattern(pattern_str);
  m_handlers[prefix] = {
    pattern,
    handler
  };
}

Status MsgParser::ParseMessage(const std::string& msg) const {
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
