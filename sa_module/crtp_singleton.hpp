#ifndef CRTP_SINGLETON_HPP_
#define CRTP_SINGLETON_HPP_
#include <type_traits>

template <typename T>
class Singleton {
public:
  template <typename... Args>
  static T* instance(Args&&... args) {
    static T instance{std::forward<Args>(args)...};

    return &instance;
  }

protected:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  virtual ~Singleton() = default;
};

#endif  // CRTP_SINGLETON_HPP_
