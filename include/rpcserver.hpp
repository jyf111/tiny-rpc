#pragma once

#include <string>

#include "asio.hpp"
#include "message.hpp"

namespace tinyrpc {

class RpcServer {
 public:
  RpcServer(uint16_t port)
      : acceptor_(io_context_,
                  asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
  RpcServer(const RpcServer& oth) = delete;
  RpcServer& operator=(const RpcServer& oth) = delete;
  ~RpcServer() { Stop(); }

  void Start() {
    Listen();
    work_thread_ = std::thread([this]() { io_context_.run(); });
  }

  void Stop() {
    if (!io_context_.stopped()) {
      io_context_.stop();
    }
    if (work_thread_.joinable()) {
      work_thread_.join();
    }
  }

  template <typename F>
  void Register(const std::string& name, F func) {
    handlers_[name] = std::bind(&RpcServer::InvokeProxy<F>, this, func,
                                std::placeholders::_1);
  }
  template <typename F, typename S>
  void Register(const std::string& name, S* obj, F func) {
    handlers_[name] = std::bind(&RpcServer::InvokeProxy<F, S>, this, func, obj,
                                std::placeholders::_1);
  }

  void UnRegister(const std::string& name) {
    if (handlers_.count(name)) {
      handlers_.erase(name);
    }
  }

  std::string Call(const std::string& name, Reader&& reader) {
    return handlers_[name](std::move(reader));
  }

 private:
  template <typename F>
  std::string InvokeProxy(F func, Reader&& reader) {
    return Invoke(func, std::move(reader));
  }
  template <typename F, typename S>
  std::string InvokeProxy(F func, S* obj, Reader&& reader) {
    return Invoke(func, obj, std::move(reader));
  }

  template <typename RType, typename... Types>
  std::string Invoke(std::function<RType(Types...)> func, Reader&& reader) {
    std::tuple<Types...> args;
    auto index_sequence =
        std::make_index_sequence<std::tuple_size_v<decltype(args)>>();
    ReadArgs(std::move(reader), args, index_sequence);
    if constexpr (std::is_same_v<void, RType>) {
      InvokeImpl(func, args, index_sequence);
      return Writer().GetString();
    } else {
      auto result = InvokeImpl(func, args, index_sequence);
      return (Writer() << result).GetString();
    }
  }
  template <typename RType, typename... Types>
  std::string Invoke(RType (*func)(Types...), Reader&& reader) {
    return Invoke(std::function<RType(Types...)>(func), std::move(reader));
  }
  template <typename RType, typename C, typename S, typename... Types>
  std::string Invoke(RType (C::*func)(Types...), S* obj, Reader&& reader) {
    std::function<RType(Types...)> wrapper = [=](Types... args) -> RType {
      return (obj->*func)(args...);
    };
    return Invoke(wrapper, std::move(reader));
  }

  template <typename... Types, std::size_t... I>
  void ReadArgs(Reader&& reader, std::tuple<Types...>& args,
                std::index_sequence<I...>) {
    static_cast<void>((reader >> ... >> std::get<I>(args)));
  }

  template <typename RType, typename... Types, std::size_t... I>
  RType InvokeImpl(std::function<RType(Types...)> func,
                   std::tuple<Types...>& args, std::index_sequence<I...>) {
    return func(std::get<I>(args)...);
  }

  void Listen() {
    acceptor_.async_accept(
        [this](std::error_code error, asio::ip::tcp::socket socket) {
          if (error) {
            return;
          }
          std::make_shared<Connection>(std::move(socket), *this)->Start();
          Listen();
        });
  }

  std::unordered_map<std::string, std::function<std::string(Reader&&)>>
      handlers_;
  // for network
  asio::io_context io_context_;
  std::thread work_thread_;
  asio::ip::tcp::acceptor acceptor_;

  class Connection : public std::enable_shared_from_this<Connection> {
   public:
    Connection(asio::ip::tcp::socket&& socket, RpcServer& server)
        : socket_(std::move(socket)), server_(server) {}
    ~Connection() { socket_.close(); }

    void Start() {
      auto self{shared_from_this()};
      asio::async_read_until(
          socket_, asio::dynamic_buffer(read_buffer_), "\r\n",
          [this, self](std::error_code error, std::size_t length) {
            if (error) {
              return;
            }
            Reader reader(read_buffer_.data(), length);
            std::string name;
            reader >> name;
            write_buffer_ = std::move(server_.Call(name, std::move(reader)));
            read_buffer_ = read_buffer_.substr(length);
            socket_.async_write_some(
                asio::buffer(write_buffer_),
                [this, self](std::error_code error, std::size_t length) {
                  if (error) {
                    return;
                  }
                  Start();
                });
          });
    }

   private:
    asio::ip::tcp::socket socket_;
    std::string read_buffer_;
    std::string write_buffer_;
    RpcServer& server_;
  };
};

}  // namespace tinyrpc