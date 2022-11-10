#pragma once
#include <string_view>
#include <WinSock2.h>

#include "ipc_headers.hpp"

class ipc_server {
public:
  ipc_server() = default;
  ~ipc_server();

  bool init(std::string_view address, unsigned short port);

  void accept();
  void disconnect();

  void send_string(std::string_view name, std::string_view string);

  template <typename T>
  void send_raw_data(std::string_view name, T data);

  template <typename T>
  raw_data_recv<T> recv_raw_data();

  string_recv recv_string();

private:
  sockaddr_in server{}, client{};
  WSADATA wsa{};
  SOCKET s{}, client_s;
};

template <typename T>
inline void ipc_server::send_raw_data(std::string_view name, T data) {
  send_string(name, std::string_view{reinterpret_cast<char*>(&data), sizeof(data)});
}

template <>
inline void ipc_server::send_raw_data(std::string_view name, std::string_view data) {
  send_string(name, data);
}

template <typename T>
inline raw_data_recv<T> ipc_server::recv_raw_data() {
  raw_data_recv<T> result{};

  auto [s, name] = recv_string();

  result.name = name;
  std::memcpy(&result.data, s.c_str(), s.size());

  return result;
}
