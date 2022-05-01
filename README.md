## tiny rpc with async call back

data struct for message exchange
```cpp
struct A {
  int x, y, z;
};
struct B {
  int x, y;
};
```
server
```cpp
class Manager {
 public:
  A Transform(B b) { return A{b.x + b.y + bias, b.x - b.y - bias, b.x * b.y * bias}; }
 private:
  int bias = 5;
};
RpcServer server(port);
Manager manager;
server.Register("Transform", &manager, &Manager::change);
server.Start();
std::this_thread::sleep_for(std::chrono::seconds(5));
server.Stop();
```

client
```cpp
RpcClient client(ip, port);
client.Start();
B b{2, 3};
client.Call<A>(
    "Transform", [&](A& a) {
      assert(a.x == b.x + b.y + 5);
      assert(a.y == b.x - b.y - 5);
      assert(a.z == b.x * b.y * 5);
    },
    b);
client.Stop();
```
由于可变参数的原因，回调函数只能放在第二个参数的位置了。回调函数的参数是RPC返回值的引用。
RPC调用的参数args支持std::is_trivially_copyable的type（底层序列化为了简化就是直接memcpy，简单考虑了大小端问题，但是结构体如果有大小端问题只能自己对每个成员转换），此外还支持各种STL容器，包括string、vector、set、map、pair......。不推荐传入C风格字符串，因为模板推导会导致退化为指针，尽量用string包装。
```cpp
Call<ReturnType>(name, [](ReturnType& result) {
  // do call back
}, args...);
```
网络库依赖于asio(https://think-async.com/Asio/)