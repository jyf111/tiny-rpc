#include "asio.hpp"
#include <iostream>
#include <thread>

using namespace asio;
io_context io;
std::string timeToString(const std::chrono::system_clock::time_point &t)
{
    std::time_t time = std::chrono::system_clock::to_time_t(t);
    std::string time_str = std::ctime(&time);
    time_str.resize(time_str.size() - 1);
    return time_str;
}

void connect_handler(std::error_code error) {
  std::this_thread::sleep_for(std::chrono::seconds(5));
  puts("connect");
}

void timeout_handler(std::error_code error) {
  puts("OK");
}

void run_io()
{
    io.run();
}
void func(int i) {
    std::cout << "func called, i= " << i << std::endl;
}
int main() {
  ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 6666);
  ip::tcp::socket socket(io);
  // socket.async_connect(ep, [](std::error_code error) {

  // })
  // socket_.async_write_some(
  //       asio::buffer(*buffer_ptr), [buffer_ptr, this, func](std::error_code error, size_t length) {
  //         if (error) {
  //           return;
  //         }
  //         std::cerr << "client write " << length << "bytes\n";
  //         asio::async_read_until(socket_, asio::dynamic_buffer(*buffer_ptr), "\r\n",
  //                         [buffer_ptr, this, func](std::error_code error, size_t length) {
  //                           if (error) {
  //                             return;
  //                           }
  //                           std::cerr << "client receive " << length << "bytes\n";
  //                           std::string message = buffer_ptr->substr(0, length);
  //                           *buffer_ptr = buffer_ptr->substr(length);
  //                           Reader reader(message);
  //                           if constexpr (std::is_same_v<RType, void>) {
  //                             func();
  //                           } else {
  //                             RType result;
  //                             reader >> result;
  //                             func(result);
  //                           }
  //                         });
  //       });
  // for ( int i = 0; i < 5; ++i)
  //       io.post(std::bind(func, i));
  // for ( int i = 5; i < 10; i+=2) {
  //       io.dispatch(std::bind(func, i));
  //       io.post(std::bind(func, i+1));
  // }
  // auto thread1 = std::thread(run_io);
  // auto thread2 = std::thread(run_io);
  // // auto thread3 = std::thread(run_io);
  // if (thread1.joinable()) thread1.join();
  // if (thread2.joinable()) thread2.join();
  // if (thread3.joinable()) thread3.join();
  // ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 6666);
  // ip::tcp::socket socket1(io);
  // ip::tcp::socket socket2(io);
  // std::cerr << socket1.is_open() << ' ' << socket2.is_open() << '\n';

  // executor_work_guard<io_context::executor_type> work_guard(io.get_executor());

  // socket1.async_connect(ep, connect_handler);
  // std::cerr << timeToString(std::chrono::system_clock::now()) << ' ' << socket1.is_open() << ' ' << socket2.is_open() << '\n';

  // socket2.async_connect(ep, connect_handler);
  // std::cerr << timeToString(std::chrono::system_clock::now()) << ' ' << socket1.is_open() << ' ' << socket2.is_open() << '\n';

  steady_timer timer(io, std::chrono::seconds(10));
  timer.async_wait(timeout_handler);

  io.run();
  io.stop();
  // auto thread1 = std::thread(run_io);
  // auto thread2 = std::thread(run_io);
  // if (thread1.joinable()) thread1.join();
  // if (thread2.joinable()) thread2.join();
  return 0;
}