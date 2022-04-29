#pragma once

#include <string>

#include "message.hpp"

namespace tinyrpc {
class RpcServer {
 public:
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

  void Call(const std::string& name, Reader&& reader) {
    handlers_[name](std::move(reader));
  }

 private:
  template <typename F>
  void InvokeProxy(F func, Reader&& reader) {
    Invoke(func, std::move(reader));
  }
  template <typename F, typename S>
  void InvokeProxy(F func, S* obj, Reader&& reader) {
    Invoke(func, obj, std::move(reader));
  }

  template <typename RType, typename... Types>
  void Invoke(std::function<RType(Types...)> func, Reader&& reader) {
    std::tuple<Types...> args;
    auto index_sequence =
        std::make_index_sequence<std::tuple_size_v<decltype(args)>>();
    ReadArgs(std::move(reader), args, index_sequence);
    auto result = InvokeImpl(func, args, index_sequence);
    Writer writer;
    (writer << result).Get();
  }

  template <typename RType, typename... Types>
  void Invoke(RType (*func)(Types...), Reader&& reader) {
    Invoke(std::function<RType(Types...)>(func), std::move(reader));
  }
  template <typename RType, typename C, typename S, typename... Types>
  void Invoke(RType (C::*func)(Types...), S* obj, Reader&& reader) {
    std::function<RType(Types...)> wrapper = [=](Types... args) -> RType {
      return (obj->*func)(args...);
    };
    Invoke(wrapper, std::move(reader));
  }
  template <typename... Types>
  void Invoke(std::function<void(Types...)> func, Reader&& reader) {
    std::tuple<Types...> args;
    auto index_sequence =
        std::make_index_sequence<std::tuple_size_v<decltype(args)>>();
    ReadArgs(std::move(reader), args, index_sequence);
    InvokeImpl(func, args, index_sequence);
    Writer writer;
    std::cout << writer.Get() << '\n';
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

  std::unordered_map<std::string, std::function<void(Reader&&)>> handlers_;
};
}  // namespace tinyrpc