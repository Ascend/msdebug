/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef ASCENDCOMMUNICATIONSERVER_H
#define ASCENDCOMMUNICATIONSERVER_H
#include "lldb/Host/Socket.h"
#include "lldb/Utility/MessageDefines.h"

#include <atomic>
#include <csignal>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace lldb_private;
using ClientMsgHandlerHook = std::function<void(Socket *, const std::string&)>;

class AscendCommunicationServer {
public:
  AscendCommunicationServer(std::size_t max_client_num, const std::string& socket_path);
  ~AscendCommunicationServer();
  bool Start();
  void Listen(Socket *client_socket);
  void SetMsgHandlerHook(ClientMsgHandlerHook &&hook);
  void Close();

private:
  std::unique_ptr<Socket> m_domain_socket;
  std::string m_socket_path;
  std::size_t m_max_client_num;
  std::thread m_accept_worker;
  ClientMsgHandlerHook m_msg_handler_hook;
  std::vector<std::thread> m_read_worker;
  std::atomic<bool> m_is_running;
};

class HandleResult : public Status {
  public:
    HandleResult() : Status() { };
    HandleResult(const Status& status) : Status(status) { }
    const std::string &GetMessage() const {
      return m_string;
    }
};

class MsgHandler {
public:
  virtual ~MsgHandler() = default;
  virtual HandleResult Parse(const std::string& msg) = 0;
  virtual HandleResult Handle() = 0;
};

class DeviceHandler : public MsgHandler {
public:
  using HandlerFunc = std::function<HandleResult(const DeviceInfoMsg&)>;

  DeviceHandler(const HandlerFunc& handler) : m_handler(handler) {}
  HandleResult Parse(const std::string& msg) override;
  HandleResult Handle() override { return m_handler(m_device_info); }

private:
  DeviceInfoMsg m_device_info{};
  HandlerFunc m_handler;
};

class KernelHandler : public MsgHandler {
public:
  using HandlerFunc = std::function<HandleResult(const KernelInfoMsg&)>;

  KernelHandler(const HandlerFunc& handler) : m_handler(handler) {}
  HandleResult Parse(const std::string& msg) override;
  HandleResult Handle() override { return m_handler(m_kernel_info); }

private:
  KernelInfoMsg m_kernel_info{};
  HandlerFunc m_handler;
};

class IpcMemHandler : public MsgHandler {
public:
  using HandlerFunc = std::function<HandleResult(const IpcMemInfoMsg &)>;

  IpcMemHandler(const HandlerFunc &handler) : m_handler(handler) {}
  HandleResult Parse(const std::string &msg) override;
  HandleResult Handle() override { return m_handler(m_ipc_mem_info); }

private:
  IpcMemInfoMsg m_ipc_mem_info{};
  HandlerFunc m_handler;
};

class MsgParser {
public:
  void Register(const std::string& prefix, std::shared_ptr<MsgHandler> handler);
  HandleResult ParseMessage(const std::string& msg) const;

private:
  struct HandlerEntry {
    std::shared_ptr<MsgHandler> handler;
  };

  std::unordered_map<std::string, HandlerEntry> m_handlers;
};

#endif //ASCENDCOMMUNICATIONSERVER_H
#endif
