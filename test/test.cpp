#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "message.hpp"
#include "rpcclient.hpp"
#include "rpcserver.hpp"
using namespace tinyrpc;

TEST_CASE("fundamental type") {
  Writer writer;
  int a = 2;
  double c = 3.24;

  struct Pod {
    int data[3] = {2, 3, 6};
  };

  Pod pod;
  short b[2] = {45, 67};
  writer << a << c << pod;

  Reader reader(writer.GetStringView());

  reader >> a >> c >> pod;

  REQUIRE(a == 2);
  REQUIRE(c == Approx(3.24));
  REQUIRE(b[0] == 45);
  REQUIRE(b[1] == 67);
  REQUIRE(pod.data[0] == 2);
  REQUIRE(pod.data[1] == 3);
  REQUIRE(pod.data[2] == 6);
}

TEST_CASE("container type") {
  int arr1[5] = {2, 3, 4, 0, 6};
  int arr2[3] = {7, 9, 10};

  int a[5] = {2, 3, 4, 0, 6};
  std::array<int, 3> b = {7, 9, 10};
  Writer writer;
  writer << a << b;

  Reader reader(writer.GetStringView());
  reader >> a >> b;
  for (int i = 0; i < 5; i++) {
    REQUIRE(a[i] == arr1[i]);
  }
  for (int i = 0; i < 3; i++) {
    REQUIRE(b[i] == arr2[i]);
  }
}

TEST_CASE("dynamic container type") {
  std::vector<int> a{2, 3, 4, 0, 6};

  std::vector<int> b;
  Writer writer;
  writer << a;

  Reader reader(writer.GetStringView());
  reader >> b;
  for (int i = 0; i < 5; i++) {
    REQUIRE(b[i] == a[i]);
  }
}

TEST_CASE("string type") {
  SECTION("1") {
    std::string message = "hello tinyrpc";
    int ext = 909;
    std::string receive;
    int ext2;
    Writer writer;
    writer << message << ext;

    Reader reader(writer.GetStringView());
    reader >> receive >> ext2;
    REQUIRE(receive == message);
    REQUIRE(ext == ext2);
  }
  SECTION("2") {
    std::string message = "add";
    std::string receive;
    Writer writer;
    writer << message;

    Reader reader(writer.GetStringView());
    reader >> receive;
    REQUIRE(receive == message);
  }
  SECTION("3") {
    const char message[] = "sub";
    std::string receive;
    Writer writer;
    writer << message;

    Reader reader(writer.GetStringView());
    reader >> receive;
    REQUIRE(receive == std::string("sub"));
  }
  SECTION("4") {
    std::string receive;
    Writer writer;
    writer << "123qwe";

    Reader reader(writer.GetStringView());
    reader >> receive;
    REQUIRE(receive == std::string("123qwe"));
  }
}

TEST_CASE("pair type") {
  std::pair<int, short> tmp{20, -5};
  std::pair<int, short> tmp2;
  Writer writer;
  writer << tmp;

  Reader reader(writer.GetStringView());
  reader >> tmp2;

  REQUIRE(tmp2.first == tmp.first);
  REQUIRE(tmp2.second == tmp.second);
}

TEST_CASE("set type") {
  REQUIRE(is_dynamic_container_v<std::set<int>> == 1);
  std::set<int> st{1, 2, 3};
  Writer writer;
  writer << st;

  Reader reader(writer.GetStringView());

  std::set<int> st2;
  reader >> st2;
  auto st2_it = st2.begin();
  REQUIRE(*st2_it++ == 1);
  REQUIRE(*st2_it++ == 2);
  REQUIRE(*st2_it++ == 3);
}

TEST_CASE("map type") {
  REQUIRE(is_dynamic_container_v<std::map<int, int>> == 1);
  std::map<int, int> mp{{1, 2}, {3, 4}};
  std::unordered_map<int, int> ump{{5, 6}, {7, 8}};
  Writer writer;
  writer << mp << ump;

  Reader reader(writer.GetStringView());

  std::map<int, int> mp2;
  std::unordered_map<int, int> ump2;
  reader >> mp2 >> ump2;
  auto mp2_it = mp2.begin();
  REQUIRE(mp2_it->first == 1);
  REQUIRE(mp2_it->second == 2);
  ++mp2_it;
  REQUIRE(mp2_it->first == 3);
  REQUIRE(mp2_it->second == 4);

  auto ump2_it = ump2.begin();
  REQUIRE(ump2_it->first == 5);
  REQUIRE(ump2_it->second == 6);
  ++ump2_it;
  REQUIRE(ump2_it->first == 7);
  REQUIRE(ump2_it->second == 8);
}

TEST_CASE("error msg") {
  class ugly {
    virtual void print() = 0;
  };
  class ugly2 : public ugly {
    void print() override { puts("OK"); }
  };
  REQUIRE(std::is_trivially_copyable_v<ugly2> == 0);
  Writer writer;
  ugly2 tmp;
  writer << tmp;
  if (!writer) {
    CHECK_FALSE(writer);
    REQUIRE(writer.GetErrorMessage() == "unsupported type in writer!");
  }

  Reader reader(writer.GetStringView());
  reader >> tmp;
  CHECK_FALSE(reader);
  REQUIRE(reader.GetErrorMessage() == "unsupported type in reader!");
}

TEST_CASE("message exchange") {
  struct input {
    int a;
    int b;
    int c;
  };
  struct output {
    int a;
    int b;
  };
  input in{1, 2, 3};
  output out{in.a + in.b * in.c, in.b - in.a * in.c};
  Writer writer;
  writer << in << out;
  Reader reader(writer.GetStringView());
  reader >> in >> out;
  CHECK(reader);
  CHECK(writer);
  REQUIRE(in.a == 1);
  REQUIRE(in.b == 2);
  REQUIRE(in.c == 3);
  REQUIRE(out.a == (in.a + in.b * in.c));
  REQUIRE(out.b == (in.b - in.a * in.c));
}

int add(int x, int y) { return x + y + 10; }
std::string echo(std::string s) { return s; }
void nothing() { return; }
class Suber {
 public:
  int sub(int x, int y) { return x - y - bias; }
  int bias = 10;
};
struct A {
  int x, y, z;
};
struct B {
  int x, y;
};
class Manager {
 public:
  A change(B b) { return A{b.x + b.y, b.x - b.y, b.x * b.y}; }
};
void RunServer() {
  RpcServer server(8888);
  server.Register("add", add);
  server.Register("echo", echo);
  Suber suber;
  server.Register("sub", &suber, &Suber::sub);
  server.Register("nothing", nothing);
  Manager manager;
  server.Register("change", &manager, &Manager::change);
  server.Start();
  std::this_thread::sleep_for(std::chrono::seconds(10));
  server.Stop();
}
TEST_CASE("server invoke") {
  std::thread server_thread(RunServer);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  RpcClient client("127.0.0.1", 8888);
  client.Start();
  std::string sb = "hello rpc";
  client.Call<std::string>(
      "echo", [&](std::string& result) { REQUIRE(result == sb); },
      sb);
  client.Call<int>(
      "sub", [](int& result) {
        REQUIRE(result == (1 - 2 - 10));
      },
      1, 2);
  client.Call<int>(
      "add", [](int& result) {
        REQUIRE(result == (1 + 2 + 10));
      },
      1, 2);
  client.Call<void>("nothing", []() { CHECK(true); });
  B b{2, 3};
  client.Call<A>(
      "change", [&](A& a) {
        REQUIRE(a.x == b.x + b.y);
        REQUIRE(a.y == b.x - b.y);
        REQUIRE(a.z == b.x * b.y);
      },
      b);

  if (server_thread.joinable()) {
    server_thread.join();
  }
  client.Stop();
}