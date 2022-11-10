#include "ipc/ipc_server.hpp"

#include <iostream>

#include "ipc/ipc_headers.hpp"

ipc_server::~ipc_server() {
  closesocket(s);
  WSACleanup();
}

bool ipc_server::init(std::string_view address, unsigned short port) {
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    return false;
  }

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    return false;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(address.data());
  server.sin_port = htons(port);

  if (bind(s, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR) {
    return false;
  }

  if (listen(s, 1) < 0) {
    return false;
  }

  return true;
}

void ipc_server::accept() {
  int client_length = sizeof(client);

  if ((client_s = ::accept(s, reinterpret_cast<sockaddr*>(&client), &client_length)) < 0) {
  }
}

void ipc_server::disconnect() {
  closesocket(client_s);
}

void ipc_server::send_string(std::string_view name, std::string_view string) {
  header h;
  h.size = name.size();

  send(client_s, reinterpret_cast<const char*>(&h), sizeof(h), 0);
  send(client_s, name.data(), name.size(), 0);

  h.size = string.size();

  send(client_s, reinterpret_cast<const char*>(&h), sizeof(h), 0);
  send(client_s, string.data(), string.size(), 0);
}

string_recv ipc_server::recv_string() {
  int bytes_received;
  string_recv result{};

  header h{};
  bytes_received = recv(client_s, reinterpret_cast<char*>(&h), sizeof(h), 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != sizeof(h)) {
    //
  }

  result.name = std::string{};
  result.name.resize(h.size);

  bytes_received = recv(client_s, result.name.data(), h.size, 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  bytes_received = recv(client_s, reinterpret_cast<char*>(&h), sizeof(h), 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  result.string.resize(h.size);

  bytes_received = recv(client_s, result.string.data(), h.size, 0);

  if (bytes_received == SOCKET_ERROR || bytes_received != h.size) {
    //
  }

  return result;
}

unsigned short ipc_server::get_bound_port() {
  sockaddr_in s_in;
  int s_in_size = sizeof(s_in);

  getsockname(s, reinterpret_cast<struct sockaddr*>(&s_in), &s_in_size); // read binding

  return ntohs(s_in.sin_port); // get the port number
}
