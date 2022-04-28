#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "message.hpp"

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

  std::string data = writer.Get();
  Reader reader(data);

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

  std::string data = writer.Get();
  Reader reader(data);
  reader >> a >> b;
  for (int i = 0; i < 5; i++) {
    REQUIRE(a[i] == arr1[i]);
  }
  for (int i = 0; i < 3; i++) {
    REQUIRE(b[i] == arr2[i]);
  }
}

TEST_CASE("dynamic container type") {
  std::vector<int> a{2, 3, 4, 0, 6};;
  std::vector<int> b;
  Writer writer;
  writer << a;

  std::string data = writer.Get();
  Reader reader(data);
  reader >> b;
  for (int i = 0; i < 5; i++) {
    REQUIRE(b[i] == a[i]);
  }
}

TEST_CASE("string type") {
  std::string message = "hello tinyrpc";
  int ext = 909;
  std::string receive;
  int ext2;
  Writer writer;
  writer << message << ext;

  std::string data = writer.Get();
  Reader reader(data);
  reader >> receive >> ext2;
  REQUIRE(receive == message);
  REQUIRE(ext == ext2);
}

TEST_CASE("pair type") {
  std::pair<int, short> tmp{20, -5};
  std::pair<int, short> tmp2;
  Writer writer;
  writer << tmp;

  std::string data = writer.Get();
  Reader reader(data);
  reader >> tmp2;

  REQUIRE(tmp2.first == tmp.first);
  REQUIRE(tmp2.second == tmp.second);
}

TEST_CASE("other stl type") {
  REQUIRE(is_dynamic_container_v<std::set<int>> == 1);
  std::set<int> st{1, 2, 3};
  Writer writer;
  writer << st;

  std::string data = writer.Get();
  Reader reader(data);

  std::set<int> st2;
  reader >> st2;
  auto st2_it = st2.begin();
  REQUIRE(*st2_it++ == 1);
  REQUIRE(*st2_it++ == 2);
  REQUIRE(*st2_it++ == 3);
}

// TEST_CASE("error type") {
//   REQUIRE(is_dynamic_container_v<std::map<int, int>> == 1);
//   std::map<int, int> mp{{1, 2}, {3, 4}};
//   Writer writer;
//   writer << mp;

//   std::string data = writer.Get();
//   Reader reader(data);

//   std::map<int, int> mp2;
//   reader >> mp2;
//   auto mp2_it = mp2.begin();
//   REQUIRE(mp2_it->first == 1);
//   REQUIRE(mp2_it->second == 2);
//   ++mp2_it;
//   REQUIRE(mp2_it->first == 3);
//   REQUIRE(mp2_it->second == 4);
// }

TEST_CASE("error msg") {
  class ugly {
    virtual void print() = 0;
  };
  class ugly2 : public ugly {
    void print() override {
      puts("OK");
    }
  };
  REQUIRE(std::is_trivially_copyable_v<ugly2> == 0);
  Writer writer;
  ugly2 tmp;
  writer << tmp;
  std::string data = writer.Get();
  Reader reader(data);
  reader >> tmp;

  if (!reader) {
    std::cerr << reader.GetErrorMessage() << '\n';
  }
}