#pragma once
#include <WinSock2.h>

#include "ipc_headers.hpp"

class ipc_client {
public:
  ipc_client() = default;
  ~ipc_client();

  bool init();
  bool connect(std::string_view hostname, unsigned short hostport);

  void send_string(std::string_view name, std::string_view string);

  template<typename T>
  void send_raw_data(std::string_view name, T data);

  template<typename T>
  raw_data_recv<T> recv_raw_data();

  string_recv recv_string();
private:
  sockaddr_in server{};
  WSADATA wsa{};
  SOCKET s{};
};

template <typename T>
inline void ipc_client::send_raw_data(std::string_view name, T data) {
  send_string(name, std::string_view{ reinterpret_cast<char*>(&data), sizeof(data) });
}

template <typename T>
inline raw_data_recv<T> ipc_client::recv_raw_data() {
  raw_data_recv<T> result{};

  auto [s, name] = recv_string();

  result.name = name;
  std::memcpy(&result.data, s.c_str(), s.size());

  return result;
}