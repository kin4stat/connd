#pragma once

#include "logger.hpp"
#include "raknet_state.hpp"
#include "raknet_utils.hpp"
#include "RakNet/InternalPacket.h"
#include "RakNet/PacketEnumerations.h"
#include "RakNet/PluginInterface.h"
#include "RakNet/RakPeer.h"
#include "ipc/ipc_server.hpp"

class RakPlugin : public PluginInterface {
  enum class EmulState {
    kNone,
    kWaitingForFirstRequest,
    kWaitingForSecondRequest,
    kWaitingForThirdRequest,
    kWaitingForAuthKey,
    kWaitingForClientJoin,
  };

public:
  bool OnInternalPacket(InternalPacket* internalPacket, unsigned frameNumber, PlayerID remoteSystemID, RakNetTime time,
                        bool isSend) override {
    if (!isSend) {
      bool deny = false;
      if ((*internalPacket->data == ID_OPEN_CONNECTION_COOKIE &&
          current_state == EmulState::kWaitingForSecondRequest) ||
        (*internalPacket->data == ID_OPEN_CONNECTION_REPLY &&
          current_state == EmulState::kWaitingForThirdRequest)) {
        deny = true;
      }
      if (deny && current_state == EmulState::kNone) {
        return true;
      }
      return !deny;
    }
    return true;
  }

  void OnDirectSocketSend(const char* data, const unsigned bitsUsed, PlayerID remoteSystemID) override {
    if (*data == ID_OPEN_CONNECTION_REQUEST) {
      if (current_state == EmulState::kWaitingForFirstRequest) {
        current_state = EmulState::kWaitingForSecondRequest;
        LOG_INFO("Pass EmulState::kWaitingForFirstRequest");

        emulate_socket_recv(rak_state.cookie_dump);
      }
      else if (current_state == EmulState::kWaitingForSecondRequest) {
        current_state = EmulState::kWaitingForThirdRequest;

        LOG_INFO("Pass EmulState::kWaitingForSecondRequest");
        emulate_socket_recv(rak_state.reply_dump);
      }
    }
  }

  bool OnSendImmediate(const char* data, const unsigned bitsUsed) override {
    if (*data == ID_CONNECTION_REQUEST && current_state == EmulState::kWaitingForThirdRequest) {
      LOG_INFO("Pass EmulState::kWaitingForThirdRequest");

      emulate_packet_recv(rak_state.auth_key_dump);

      LOG_INFO("Emulating Authkey");

      auto rak = get_pure_rak_client();
      auto rss = rak->remoteSystemList;

      RakNet::BitStream in_bs(reinterpret_cast<unsigned char*>(rak_state.conn_accept_dump.data()),
                              rak_state.conn_accept_dump.size(), false);


      PlayerID externalID{};

      in_bs.IgnoreBits(8);
      in_bs.Read(externalID.binaryAddress);
      in_bs.Read(externalID.port);

      rss->myExternalPlayerId = externalID;
      rss->connectMode = RakPeer::RemoteSystemStruct::CONNECTED;

      rss->reliabilityLayer.SetEncryptionKey(nullptr);

      emulate_packet_recv(rak_state.conn_accept_dump);

      LOG_INFO("Emulating Connection accept");

      rss->staticData.Reset();
      rss->staticData.Write(rak_state.static_data_dump.data() + 1,
                            rak_state.static_data_dump.size() - 1);

      emulate_packet_recv(rak_state.static_data_dump);

      LOG_INFO("Emulating static data");

      current_state = EmulState::kNone;

      auto& rel_level = get_pure_rak_client()->remoteSystemList->reliabilityLayer;

      std::ranges::copy(rak_state.waitingForOrderedPacketWriteIndex,
                        std::begin(rel_level.waitingForOrderedPacketWriteIndex));
      std::ranges::copy(rak_state.waitingForSequencedPacketWriteIndex,
                        std::begin(rel_level.waitingForSequencedPacketWriteIndex));
      std::ranges::copy(rak_state.waitingForOrderedPacketReadIndex,
                        std::begin(rel_level.waitingForOrderedPacketReadIndex));
      std::ranges::copy(rak_state.waitingForSequencedPacketReadIndex,
                        std::begin(rel_level.waitingForSequencedPacketReadIndex));

      rel_level.receivedPacketsBaseIndex = rak_state.received_packets_base_index;
      rel_level.messageNumber = rak_state.send_number;

      LOG_INFO("Restored reliability level");

      LOG_INFO("Connected");

      return false;
    }
    return true;
  }

  void OnRpcReceive(unsigned char rpc_id, RPCParameters* params) override {
    if (params->input) {
      std::vector<char> this_rpc(params->numberOfBitsOfData / 8 + 2);
      this_rpc[0] = static_cast<char>(rpc_id);
      std::memcpy(&this_rpc[1], params->input, params->numberOfBitsOfData / 8 + 1);

      rpc_queue.emplace_back(std::move(this_rpc));
    }
    else {
      std::vector<char> this_rpc{static_cast<char>(rpc_id)};

      rpc_queue.emplace_back(std::move(this_rpc));
    }
  }

  void send_rpcs(ipc_server* server) {
    while (true) {
      auto [name, data] = server->recv_string();
      if (name == "get_rpcs" && data == "get_rpcs") {
        break;
      }
    }

    for (auto& rpc : rpc_queue) {
      server->send_raw_data("stop_receiving", 0);
      server->send_string("raw_rpc", std::string_view{rpc.data(), rpc.size()});
    }
    server->send_raw_data("stop_receiving", 1);
  }

  std::size_t get_rpc_count() const {
    return rpc_queue.size();
  }

private:
  std::vector<std::vector<char>> rpc_queue;
  EmulState current_state{EmulState::kWaitingForFirstRequest};
};
