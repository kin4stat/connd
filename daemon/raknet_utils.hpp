#pragma once
#include "rakclient.hpp"
#include "RakNet/RakClient.h"

RakClient* get_pure_rak_client() {
  return static_cast<RakClient*>(g_rak_client);
}

void emulate_packet_recv(std::span<char> packet_data) {
  const auto packet_size = packet_data.size();
  const auto packet_data_alloc = new unsigned char[packet_size];
  std::memcpy(packet_data_alloc, packet_data.data(), packet_data.size());
  Packet* packet = AllocPacket(packet_size, packet_data_alloc);

  packet->bitSize = packet_size * 8;
  packet->playerId = rak_state.server_pid;
  packet->playerIndex = rak_state.pindex;

  get_pure_rak_client()->AddPacketToProducer(packet);
}

void emulate_socket_recv(std::span<char> packet_data) {
  ProcessNetworkPacket(rak_state.server_pid.binaryAddress,
                       rak_state.server_pid.port,
                       packet_data.data(),
                       packet_data.size(),
                       get_pure_rak_client());
}