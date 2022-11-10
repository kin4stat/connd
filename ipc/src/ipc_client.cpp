#include "ipc/ipc_client.hpp"
#include "ipc/ipc_headers.hpp"

ipc_client::~ipc_client() {
  closesocket(s);
  WSACleanup();
}

bool ipc_client::connect(std::string_view hostname, unsigned short hostport) {
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(hostname.data());
  server.sin_port = htons(hostport);

  if (::connect(s, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
    return false;
  }
  return true;
}

void ipc_client::send_string(std::string_view name, std::string_view string) {
  header h;
  h.size = name.size();

  send(s, reinterpret_cast<const char*>(&h), sizeof(h), 0);
  send(s, name.data(), name.size(), 0);

  h.size = string.size();

  send(s, reinterpret_cast<const char*>(&h), sizeof(h), 0);
  send(s, string.data(), string.size(), 0);
}

string_recv ipc_client::recv_string() {
  int bytes_received;
  string_recv result{};

  header h{};
  bytes_received = recv(s, reinterpret_cast<char*>(&h), sizeof(h), 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != sizeof(h)) {
    //
  }

  result.name = std::string{};
  result.name.resize(h.size);

  bytes_received = recv(s, result.name.data(), h.size, 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  bytes_received = recv(s, reinterpret_cast<char*>(&h), sizeof(h), 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  result.string.resize(h.size);

  bytes_received = recv(s, result.string.data(), h.size, 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  return result;
}

bool ipc_client::init() {
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    return false;
  }
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    return false;
  }

  return true;
}
