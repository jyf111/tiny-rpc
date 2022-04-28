#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

namespace tinyrpc {
class Message;
class Writer;
class Reader;

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
  uint32_t Size() const { return header_.size; }

  template <typename U, typename V>
  void ByteSwap(U* l, V* r) {
    auto st = reinterpret_cast<char*>(l);
    auto ed = reinterpret_cast<char*>(r);
    --ed;
    while (st < ed) {
      std::swap(*st, *ed);
      ++st, --ed;
    }
  }

  template <typename T>
  std::string Debug(T* x) {
    std::ostringstream oss;
    auto ptr = reinterpret_cast<char*>(x);
    for (size_t i = 0; i < sizeof(T); ++i) {
      oss << DebugByte(*ptr++) << ' ';
    }
    return oss.str();
  }

 protected:
  Message() {
    header_ = {0U, 0U, MessageState::Success};

    uint16_t value = 0x01;
    auto least_significant_byte = *reinterpret_cast<uint8_t*>(&value);
    endian_ = least_significant_byte == 0x01 ? endian::little : endian::big;
  }

  void SetError() { header_.state = MessageState::Error; }

  bool IsError() const { return header_.state == MessageState::Error; }

  std::string DebugByte(int x) {
    std::ostringstream oss;
    for (int i = 0; i < 8; i++) {
      oss << ((x >> i) & 1);
    }
    return oss.str();
  }

  enum class MessageState : bool { Success, Error };
  struct header {
    uint32_t identifier;
    uint32_t size;
    MessageState state;
  };
  enum class endian { little, big };

  header header_;
  endian endian_;

  static constexpr uint32_t Magic =
      0xc2a9c9a7;  // echo -n tinyrpc | md5sum: c2a9c9a7fd9ab6f3d6d18bfc49eb7f21
};

class Writer : public Message {
 public:
  Writer() : data_(sizeof(header), 0) {}
  Writer(const Writer& oth) = delete;
  Writer& operator=(const Writer& oth) = delete;

  std::string Get() {
    if (!header_.identifier) {
      WriteHeader();
    }
    return data_;
  }

  template <typename T>
  Writer& operator<<(const T& obj) {
    if (IsError()) {
      return *this;
    }

    if constexpr (is_dynamic_container_v<T>) {
      return WriteDynamicArray(obj);
    } else if constexpr (is_container_v<T>) {
      return WriteArray(obj);
    } else if constexpr (std::is_trivially_copyable_v<T>) {
      int pre_size = data_.size();
      data_.resize(pre_size + sizeof(T));

      std::memcpy(data_.data() + pre_size, &obj, sizeof(T));
      if (endian_ == endian::big) {
        ByteSwap(data_.data() + pre_size, data_.data() + data_.size());
      }

      return *this;
    }
    SetError();
    data_.resize(sizeof(header));
    data_ += "unsupported type.";
    return *this;
  }

  template <typename U, typename V>
  Writer& operator<<(std::pair<U, V>& obj) {
    if (IsError()) {
      return *this;
    }

    (*this) << obj.first << obj.second;
    return *this;
  }

 private:
  void WriteHeader() {
    header_.identifier = Magic;
    header_.size = data_.size() - sizeof(header);

    std::memcpy(data_.data(), &header_, sizeof(header));

    if (endian_ == endian::big) {
      ByteSwap(data_.data(), data_.data() + sizeof(header_.identifier));
      ByteSwap(data_.data() + sizeof(header_.identifier),
               data_.data() + sizeof(header));
    }
  }

  template <typename T>
  Writer& WriteArray(const T& obj) {
    for (const auto& iter : obj) {
      (*this) << iter;
    }
    return *this;
  }

  template <typename T>
  Writer& WriteDynamicArray(const T& obj) {
    (*this) << std::size(obj);
    for (const auto& iter : obj) {
      (*this) << iter;
    }
    return *this;
  }

  std::string data_;
};

class Reader : public Message {
 public:
  explicit Reader(const std::string& data)
      : Message(), data_(data.data(), data.size()) {
    ReadHeader(data.size());
  }
  Reader(const Reader& oth) = delete;
  Reader& operator=(const Reader& oth) = delete;

  std::string GetErrorMessage() const {
    return *error_msg_;
  }

  operator bool() const {
    return error_msg_ == std::nullopt;
  }

  template <typename T>
  Reader& operator>>(T& obj) {
    if (IsError()) {
      return *this;
    }

    if constexpr (is_dynamic_container_v<T>) {
      return ReadDynamicArray(obj);
    } else if constexpr (is_container_v<T>) {
      return ReadArray(obj);
    } else if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(&obj, data_.data(), sizeof(T));
      if (endian_ == endian::big) {
        ByteSwap(&obj, &obj + 1);
      }

      data_.remove_prefix(sizeof(T));
      return *this;
    }
    SetError();
    error_msg_ = "unsupported type.";
    return *this;
  }

  template <typename U, typename V>
  Reader& operator>>(std::pair<U, V>& obj) {
    if (IsError()) {
      return *this;
    }

    (*this) >> obj.first >> obj.second;
    return *this;
  }

 private:
  void ReadHeader(uint32_t size) {
    std::memcpy(&header_, data_.data(), sizeof(header));

    if (endian_ == endian::big) {
      ByteSwap(&header_, &header_.size);
      ByteSwap(&header_.size, &header_ + 1);
    }
    data_.remove_prefix(sizeof(header));
    if (IsError()) {
      error_msg_ = std::string(data_);
    } else if (header_.identifier != Magic) {
      SetError();
      error_msg_ = "unsupported message type.";
    } else if (Size() != size - sizeof(header)) {
      SetError();
      error_msg_ = "error message length.";
    }
  }

  template <typename T>
  Reader& ReadArray(T& obj) {
    for (auto& iter : obj) {
      (*this) >> iter;
    }
    return *this;
  }

  template <typename T>
  Reader& ReadDynamicArray(T& obj) {
    size_t sz;
    (*this) >> sz;
    for (size_t i = 0; i < sz; ++i) {
      typename T::value_type value;
      (*this) >> value;
      obj.insert(obj.end(), value);
    }
    return *this;
  }

  std::string_view data_;
  std::optional<std::string> error_msg_ = std::nullopt;
};

}  // namespace tinyrpc