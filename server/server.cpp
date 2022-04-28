#include "asio.hpp"
#include <iostream>
#include "serializer.hpp"
char buffer[1024];
void read(asio::ip::tcp::socket& socket) {
  socket.async_read_some(asio::buffer(buffer), [&](std::error_code error, std::size_t length) {
    if (!error) {
      std::cout << "read " << length << " bytes\n";
      buffer[length] = 0;
      std::cout << buffer << '\n';
      read(socket);
    }
  });
}
int main() {
  asio::io_context io_context(1);

  asio::ip::tcp::acceptor acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 6666));

  asio::ip::tcp::socket socket(io_context);

  io_context.run();
  puts("HERE");
  acceptor.async_accept(socket, [&](std::error_code error) {
    if (error) {
      socket.close();
    } else {
      puts("OK!!");
    }
  });
  // read
  // while (true) {
  //   asio::error_code error;
  //   std::array<char, 512> buffer;
  //   std::cerr << "begin read!\n";
  //   size_t len = socket.read_some(asio::buffer(buffer), error);
  //   std::cerr << "end read!\n";
  //   std::cout.write(buffer.data(), len);
  //   if (error == asio::error::eof)
  //     break; // Connection closed cleanly by peer.
  // }

  read(socket);
  puts("HAHA");
  io_context.run();
  std::cout << "Server finish work" << std::endl;
  return 0;
}