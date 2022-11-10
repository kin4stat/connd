#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "RakNet/NetworkTypes.h"
#include <array>

class raknet_state {
public:
  raknet_state() = default;
  ~raknet_state() = default;


  std::string hostname;
  unsigned short local_port;
  unsigned short received_packets_base_index;
  unsigned short send_number;

  PlayerID server_pid;
  PlayerIndex pindex;

  std::vector<char> cookie_dump;
  std::vector<char> reply_dump;
  std::vector<char> auth_key_dump;
  std::vector<char> conn_accept_dump;
  std::vector<char> static_data_dump;

  std::array<unsigned short, 32> waitingForOrderedPacketWriteIndex;
  std::array<unsigned short, 32> waitingForSequencedPacketWriteIndex;
  std::array<unsigned short, 32> waitingForOrderedPacketReadIndex;
  std::array<unsigned short, 32> waitingForSequencedPacketReadIndex;

  std::uint32_t timeout = 3 * 60;
};

inline raknet_state rak_state{};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerID,
                                   binaryAddress,
                                   port)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(raknet_state,
                                   hostname,
                                   local_port,
                                   received_packets_base_index,
                                   send_number,
                                   server_pid,
                                   pindex,
                                   cookie_dump,
                                   reply_dump,
                                   auth_key_dump,
                                   conn_accept_dump,
                                   static_data_dump,
                                   waitingForOrderedPacketReadIndex,
                                   waitingForOrderedPacketWriteIndex,
                                   waitingForSequencedPacketReadIndex,
                                   waitingForSequencedPacketWriteIndex,
                                   timeout)
