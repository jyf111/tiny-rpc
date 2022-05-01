#pragma once

#include <any>
#include <atomic>

#include "message.hpp"
#include "rpcserver.hpp"

namespace tinyrpc {

class RpcClient {
 public:
  RpcClient(const std::string& ip, uint16_t port)
      : endpoint_(asio::ip::address::from_string(ip), port),
        socket_(io_context_),
        work_guard_(io_context_.get_executor()) {}
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
    if (!io_context_.stopped()) {
      io_context_.stop();
    }
    if (work_thread_.joinable()) {
      work_thread_.join();
    }
  }

  template <typename RType, typename... Types, typename F>
  void Call(const std::string& name, F func, Types... args) {
    CallImpl<RType>(name, func, args...);
  }

 private:
  template <typename RType, typename... Types, typename F>
  void CallImpl(const std::string& name, F func, Types... args) {
    Writer writer;
    writer << name;
    static_cast<void>((writer << ... << args));
    lock_.lock();
    write_buffer_ = std::move(writer.GetString());
    socket_.async_write_some(
        asio::buffer(write_buffer_),
        [this, func, name](std::error_code error, size_t length) {
          if (error) {
            return;
          }
          asio::async_read_until(
              socket_, asio::dynamic_buffer(read_buffer_), "\r\n",
              [this, func](std::error_code error, size_t length) {
                if (error) {
                  return;
                }
                Reader reader(read_buffer_.data(), length);
                if constexpr (std::is_same_v<RType, void>) {
                  func();
                } else {
                  RType result;
                  reader >> result;
                  func(result);
                }
                read_buffer_ = read_buffer_.substr(length);
                lock_.unlock();
              });
        });
  }

  std::string write_buffer_;
  std::string read_buffer_;
  std::mutex lock_;  // manage the buffer
  // network
  asio::ip::tcp::endpoint endpoint_;
  asio::io_context io_context_;
  asio::ip::tcp::socket socket_;
  std::thread work_thread_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
};
}  // namespace tinyrpc