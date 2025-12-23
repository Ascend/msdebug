#ifdef MS_DEBUGGER
#include "AscendCommunicationClient.h"
#include "lldb/Host/posix/AscendDomainSocket.h"
#include "rt_stub_log.h"
#include <memory>
#include <mutex>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

std::mutex AscendCommunicationClient::device_mutex_;

AscendCommunicationClient::AscendCommunicationClient()
{
  constexpr int retryLimit = 50;    // 链接重试次数上限为50
  char *fdBuf = std::getenv("MSOP_SOCKET_PATH");
  if (fdBuf == nullptr) {
    RT_STUB_LOG_ERROR("Get socket path failed!\n");
    ThrowErrorCode(SOCKET_PATH_NOT_FOUND_ERR);
  }
  std::string socketPath(fdBuf);
  domain_client_ = std::make_unique<AscendDomainSocket>(true, false);
  int retryTimes = 0;
  while (domain_client_ && domain_client_->Connect(socketPath).Fail() && retryTimes <= retryLimit) {
    // server 启动前 client 会连接失败，等待 100ms 后重试
    constexpr uint64_t connectRetryDuration = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(connectRetryDuration));
    retryTimes++;
  }
  auto timeout = timeval {};
  timeout.tv_sec = 10; // 暂设置read超时时间为1s
  timeout.tv_usec = 0;
  if (setsockopt(domain_client_->GetNativeSocket(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
    return;
  }
  if (retryTimes > retryLimit) {
    RT_STUB_LOG_ERROR("Conntect failed!, retryTimes: %d\n", retryTimes);
    return;
  }
}

AscendCommunicationClient &AscendCommunicationClient::GetInstance(std::int32_t deviceId)
{
  static std::unordered_map<int32_t, std::unique_ptr<AscendCommunicationClient>> devices;
  std::lock_guard<std::mutex> guard(device_mutex_);
  auto it = devices.find(deviceId);
  if (it != devices.end()) {
    return *it->second; // Ensure the device is not destroyed
  }
  devices[deviceId] = std::make_unique<AscendCommunicationClient>();
  RT_STUB_LOG_INFO("New Client, device id: %d\n", deviceId);
  return *devices[deviceId]; // Return the new instance
}

// 通信一来一回，避免两边都在写或者单边多线程写导致挂掉
int AscendCommunicationClient::SendAndWaitResponse(std::string const &sendData, std::string& resp)
{
    std::lock_guard<std::mutex> lockGuard(mtx_);
    int ret = Write(sendData);
    if (ret <= 0) {
        RT_STUB_LOG_ERROR("Write error: %s", strerror(errno));
        return ret;
    }
    ret = Read(resp);
    if (ret <= 0) {
        RT_STUB_LOG_ERROR("Read error: %s", strerror(errno));
    }
    return ret;
}

int AscendCommunicationClient::Write(const std::string& msg)
{
  size_t write_size = msg.size();
  int32_t sentBytes = -1;
  if (domain_client_) {
    sentBytes = domain_client_->Write(msg.data(), write_size).Success() ? write_size : -1;
  }
  return sentBytes;
}

int AscendCommunicationClient::Read(std::string& msg) const
{
  static constexpr std::size_t MAX_SIZE = 1024ULL;
  std::vector<char> buffer(MAX_SIZE);
  size_t read_size = MAX_SIZE;
  int32_t len = -1;
  if (domain_client_) {
    len = domain_client_->Read(buffer.data(), read_size).Success() ? read_size : -1;
    msg.assign(buffer.data(), read_size);
  }
  return len;
}
#endif
