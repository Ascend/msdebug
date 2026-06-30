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

using namespace lldb_private;
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
  timeout.tv_sec = 30; // 设置read超时时间为30s, 30MB传输需要40s，后面会重试3次
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
size_t AscendCommunicationClient::SendAndWaitResponse(std::string const &sendData, std::string& resp)
{
    std::lock_guard<std::mutex> lockGuard(mtx_);
    size_t ret = Write(sendData);
    if (ret == 0) {
        RT_STUB_LOG_ERROR("Write error: %s", strerror(errno));
        return ret;
    }
    ret = Read(resp);
    if (ret == 0) {
        RT_STUB_LOG_ERROR("Read error: %s", strerror(errno));
    }
    return ret;
}

size_t AscendCommunicationClient::Write(const std::string& msg)
{
  size_t src_len = msg.size();
  size_t num_bytes;
  size_t total_written = 0;
  Status status;
  if (domain_client_) {
    do {
      num_bytes = src_len - total_written;
      status = domain_client_->Write(msg.data() + total_written, num_bytes);
      total_written += num_bytes;
    } while (status.Success() && total_written < src_len);
    if (total_written != src_len) {
      RT_STUB_LOG_ERROR("total_written=%lu bytes, need write=%lu bytes", total_written, src_len);
      return 0;
    }
  }
  return total_written;
}

size_t AscendCommunicationClient::Read(std::string& msg) const
{
  static constexpr std::size_t MAX_SIZE = 1024ULL;
  std::vector<char> buffer(MAX_SIZE);
  size_t read_size = 0;
  Status status;
  if (domain_client_) {
    const size_t max_response_retries = 3;
    for (size_t i = 0; i < max_response_retries; ++i) {
      read_size = MAX_SIZE;
      status = domain_client_->Read(buffer.data(), read_size);
      if (status.Fail()) {
        RT_STUB_LOG_WARNING("read failed, error: %s", status.AsCString());
      }
      if (read_size > 0) {
        msg.assign(buffer.data(), read_size);
        break;
      }
    }
  }
  return read_size;
}
#endif
