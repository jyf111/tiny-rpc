#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

namespace tinyrpc {
namespace {
template <class Default, class AlwaysVoid, template <class...> class Op,
          class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
  using type = Op<Args...>;
};

struct nonesuch {};

template <template <class...> class Op, class... Args>
using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <typename T>
using detect_begin_t = decltype(std::begin(std::declval<T>()));
template <typename T>
using detect_end_t = decltype(std::end(std::declval<T>()));
template <typename T>
using detect_size_t = decltype(std::size(std::declval<T>()));
template <typename T>
using detect_insert_t = decltype(std::declval<T>().insert(
    std::begin(std::declval<T>()), typename T::value_type{}));

template <typename T>
constexpr bool is_container_v =
    std::conjunction_v<is_detected<detect_begin_t, const T>,
                       is_detected<detect_end_t, const T>,
                       is_detected<detect_size_t, const T>>;
template <typename T>
constexpr bool is_dynamic_container_v = std::conjunction_v<
    is_detected<detect_begin_t, const T>, is_detected<detect_end_t, const T>,
    is_detected<detect_size_t, const T>, is_detected<detect_insert_t, T>>;

}  // namespace

class Message {
 public:
  static constexpr uint32_t HeaderLength = 8;  // sizeof(header);
  static constexpr uint32_t MaxLength = 1 << 12;

  uint32_t Length() const { return header_.length; }

  const std::string& GetErrorMessage() const {
    assert(IsError());
    return *error_;
  }

  operator bool() const { return !IsError(); }

  template <typename U, typename V>
  static void ByteSwap(U* l, V* r) {
    auto st = reinterpret_cast<char*>(l);
    auto ed = reinterpret_cast<char*>(r) - 1;
    while (st < ed) {
      std::swap(*st, *ed);
      ++st, --ed;
    }
  }

  template <typename T>
  static std::string Debug(T* x) {
    std::ostringstream oss;
    auto ptr = reinterpret_cast<const char*>(x);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
      oss << DebugByte(*ptr++) << ' ';
    }
    return oss.str();
  }

  static std::string DebugByte(const char x) {
    std::ostringstream oss;
    for (int i = 0; i < 8; i++) {
      oss << ((x >> i) & 1);
    }
    return oss.str();
  }

 protected:
  Message() : header_{0U, 0U}, error_(std::nullopt) {
    uint16_t value = 0x01;
    auto least_significant_byte = *reinterpret_cast<uint8_t*>(&value);
    endian_ = least_significant_byte == 0x01 ? endian::little : endian::big;
  }

  void SetError(const std::string& err_message) { error_ = err_message; }

  bool IsError() const { return error_ != std::nullopt; }

  struct header {
    uint32_t identifier;
    uint32_t length;
  };
  enum class endian { little, big };

  header header_;
  std::optional<std::string> error_;
  endian endian_;

  static constexpr uint32_t Magic =
      0xc2a9c9a7;  // echo -n tinyrpc | md5sum: c2a9c9a7fd9ab6f3d6d18bfc49eb7f21
};

class Writer : public Message {
 public:
  Writer() : data_(sizeof(header), '\0') {}
  Writer(const Writer& oth) = delete;
  Writer& operator=(const Writer& oth) = delete;

  std::string_view GetStringView() {
    if (!header_.identifier) {
      WriteHeader();
    }
    return std::string_view(data_);
  }
  const std::string& GetString() {
    if (!header_.identifier) {
      WriteHeader();
    }
    return data_;
  }

  template <typename T>
  Writer& operator<<(const T& obj) {
    if (IsError()) {  // if error occured before, just do nothing
      return *this;
    }
    if constexpr (std::is_pointer_v<T>) {  // do not use raw c string, it will
                                           // decay to char*
      SetError("pointer is dangerouse!");
      return *this;
    } else if constexpr (is_container_v<T>) {
      return WriteArray(obj);
    } else if constexpr (std::is_trivially_copyable_v<T>) {
      int pre_size = data_.size();
      data_.resize(pre_size + sizeof(T));

      std::memcpy(data_.data() + pre_size, &obj, sizeof(T));
      if (endian_ == endian::big) {
        if (!std::is_class_v<T>) {  // for struct, it is wrong
          ByteSwap(data_.data() + pre_size, data_.data() + data_.size());
        }
      }

      return *this;
    }
    SetError("unsupported type in writer!");
    return *this;
  }

  template <typename U, typename V>
  Writer& operator<<(std::pair<U, V>& obj) {
    if (IsError()) {
      return *this;
    }
    return (*this) << obj.first << obj.second;
  }

 private:
  void WriteHeader() {
    header_.identifier = Magic;
    header_.length = data_.size() - sizeof(header);

    std::memcpy(data_.data(), &header_, sizeof(header));

    if (endian_ == endian::big) {
      ByteSwap(data_.data(), data_.data() + sizeof(header_.identifier));
      ByteSwap(data_.data() + sizeof(header_.identifier),
               data_.data() + sizeof(header));
    }
  }

  template <typename T>
  Writer& WriteArray(const T& obj) {
    (*this) << std::size(obj);
    for (const auto& iter : obj) {
      (*this) << iter;
    }
    return *this;
  }

  Writer& WriteArray(const char* const& obj) {
    return WriteArray(std::string(obj));
  }

  std::string data_;
};

class Reader : public Message {
 public:
  Reader(std::string_view&& data, bool contain_header = true)
      : Message(), data_(std::move(data)) {
    if (contain_header) {
      ReadHeader();
    }
  }
  Reader(const char* data, std::size_t length, bool contain_header = true)
      : Message(), data_(data, length) {
    if (contain_header) {
      ReadHeader();
    }
  }
  Reader(const Reader& oth) = delete;
  Reader& operator=(const Reader& oth) = delete;

  template <typename T>
  Reader& operator>>(T& obj) {
    if (IsError()) {
      return *this;
    }
    if constexpr (std::is_pointer_v<T>) {
      SetError("pointer is dangerouse!");
      return *this;
    } else if constexpr (is_dynamic_container_v<T>) {
      return ReadDynamicArray(obj);
    } else if constexpr (is_container_v<T>) {
      return ReadArray(obj);
    } else if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(&obj, data_.data(), sizeof(T));
      if (endian_ == endian::big) {
        if (!std::is_class_v<T>) {
          ByteSwap(&obj, &obj + 1);
        }
      }
      data_.remove_prefix(sizeof(T));
      return *this;
    }
    SetError("unsupported type in reader!");
    return *this;
  }

  template <typename U, typename V>
  Reader& operator>>(std::pair<U, V>& obj) {
    if (IsError()) {
      return *this;
    }
    return (*this) >> obj.first >> obj.second;
  }

  template <typename U, typename V>
  Reader& operator>>(std::map<U, V>& obj) {
    if (IsError()) {
      return *this;
    }
    obj.clear();
    std::size_t sz;
    (*this) >> sz;
    for (std::size_t i = 0; i < sz; i++) {
      std::pair<U, V> value;
      (*this) >> value;
      obj.emplace_hint(obj.end(), value);
    }
    return *this;
  }
  template <typename U, typename V>
  Reader& operator>>(std::unordered_map<U, V>& obj) {
    if (IsError()) {
      return *this;
    }
    obj.clear();
    std::size_t sz;
    (*this) >> sz;
    for (std::size_t i = 0; i < sz; i++) {
      std::pair<U, V> value;
      (*this) >> value;
      obj.emplace(value);
    }
    return *this;
  }

 private:
  void ReadHeader() {
    std::memcpy(&header_, data_.data(), sizeof(header));

    if (endian_ == endian::big) {
      ByteSwap(&header_, &header_.length);
      ByteSwap(&header_.length, &header_ + 1);
    }
    data_.remove_prefix(sizeof(header));
    if (header_.identifier != Magic) {
      SetError("unsupported message type!");
    }
  }

  template <typename T>
  Reader& ReadArray(T& obj) {
    std::size_t sz;
    (*this) >> sz;
    for (auto& iter : obj) {
      (*this) >> iter;
    }
    return *this;
  }
  template <typename T>
  Reader& ReadDynamicArray(T& obj) {
    std::size_t sz;
    (*this) >> sz;
    typename T::value_type value;
    for (std::size_t i = 0; i < sz; ++i) {
      (*this) >> value;
      obj.insert(obj.end(), value);
    }
    return *this;
  }

  std::string_view data_;
};

}  // namespace tinyrpc