#pragma once

#include "message.hpp"
#include "rpcserver.hpp"

namespace tinyrpc {
class RpcClient {
 public:
  RpcClient(const std::string& ip, uint16_t port)
      : endpoint_(asio::ip::address::from_string(ip), port),
        socket_(io_context_), work_guard_(io_context_.get_executor()) {}
  RpcClient(const RpcClient& oth) = delete;
  RpcClient& operator=(const RpcClient& oth) = delete;
  ~RpcClient() { Stop(); }

  void Start() {
    socket_.async_connect(endpoint_, [](std::error_code error) {
      if (error) {
        return;
      }
    });
    work_thread_ = std::thread([this]() { io_context_.run(); });
  }

  void Stop() {
    if (socket_.is_open()) {
      asio::post(io_context_, [this]() { socket_.close(); });
    }
    io_context_.stop();
    if (work_thread_.joinable()) {
      work_thread_.join();
    }
  }
  template <typename RType, typename... Types, typename F>
  void Call(const std::string& name, F func, Types... args) {
    Writer writer;
    writer << name;
    std::shared_ptr<std::string> buffer_ptr = std::make_shared<std::string>();
    *buffer_ptr = std::move((writer << ... << args).GetString());
    // for (int i=0; i<(int)buffer_.size(); ++i) {
    //   std::cerr << Message::DebugByte(buffer_[i]) << ' ';
    // }
    // std::cerr << '\n';
    socket_.async_write_some(
        asio::buffer(*buffer_ptr), [buffer_ptr, this, func](std::error_code error, size_t length) {
          if (error) {
            return;
          }
          buffer_ptr->clear();
          // std::cerr << "client write " << length << "bytes\n";
          asio::async_read_until(socket_, asio::dynamic_buffer(*buffer_ptr), "\r\n",
                          [buffer_ptr, this, func](std::error_code error, size_t length) {
                            if (error) {
                              return;
                            }
                            // std::cerr << "client receive " << length << "bytes\n";
                            std::string message = buffer_ptr->substr(0, length);
                            *buffer_ptr = buffer_ptr->substr(length);
                            Reader reader(message);
                            if constexpr (std::is_same_v<RType, void>) {
                              func();
                            } else {
                              RType result;
                              reader >> result;
                              func(result);
                            }
                          });
        });
  }

 private:
  asio::ip::tcp::endpoint endpoint_;
  asio::io_context io_context_;
  asio::ip::tcp::socket socket_;
  std::thread work_thread_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
};
}  // namespace tinyrpc