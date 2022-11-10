#pragma once

#include <string_view>
#include <WinSock2.h>

class ipc_client {
public:
  ipc_client() = default;
  ~ipc_client();

  void init();
  void connect(std::string_view hostname, unsigned short hostport);

private:
  sockaddr_in server{};
  WSADATA wsa{};
  SOCKET s{};
};
