#include "asio.hpp"
#include <iostream>

int main() {
  asio::io_context io_context(1);

  asio::ip::tcp::socket socket(io_context);
  socket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 6666));
  asio::error_code ignored_error;
  if (socket.is_open()) {
    for (int i=0; i<5; i++) {
      std::string message;
      std::cin >> message;
      message += '\n';
      socket.write_some(asio::buffer(message), ignored_error);
    }
  }
  return 0;
}