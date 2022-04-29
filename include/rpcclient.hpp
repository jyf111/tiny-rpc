#pragma once

#include "message.hpp"

namespace tinyrpc {
class RpcClient {
public:
  template <typename RType, typename... Types>
  RType Call(const std::string& name, Types... args) {
    Writer writer;
    std::string data = std::move((writer << ... << args).Get());
  }
private:
};
}