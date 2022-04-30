#include "asio.hpp"
#include <iostream>
using namespace asio;

std::string timeToString(const std::chrono::system_clock::time_point &t)
{
    std::time_t time = std::chrono::system_clock::to_time_t(t);
    std::string time_str = std::ctime(&time);
    time_str.resize(time_str.size() - 1);
    return time_str;
}

int main() {
  io_context io;
  ip::tcp::endpoint ep(ip::tcp::v4(), 6666);
  ip::tcp::acceptor acceptor(io, ep);
  while ( true) {
    ip::tcp::socket socket(io);
    puts("Waiting...");
    acceptor.accept(socket);
    std::cerr << timeToString(std::chrono::system_clock::now()) << '\n';
  }
  return 0;
}