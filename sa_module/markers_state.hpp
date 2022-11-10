#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "BitStream.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "rpc_handler.hpp"
#include "sampapi/sampapi.h"
#include "sampapi/CNetGame.h"

class rpc_handler;

class markers_state : public Singleton<markers_state> {
public:
  markers_state() = default;
  ~markers_state() override = default;

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_ScrSetMapIcon ||
        rpc == RPCEnumeration::RPC_ScrDisableMapIcon) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto& pool = to["markers"];

    for (auto& marker : markers) {
      if (!marker._enabled) continue;

      pool.emplace_back(marker);
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto netgame = RefNetGame(version);

    for (auto& arr : from["markers"]) {
      auto this_state = arr.get<Marker>();

      netgame->CreateMarker(this_state.icon_id,
                            this_state.position,
                            this_state.type,
                            this_state.color,
                            this_state.style);

      markers[this_state.icon_id] = this_state;
      markers[this_state.icon_id]._enabled = true;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    if (rpc_id == RPCEnumeration::RPC_ScrSetMapIcon) {
      std::uint8_t icon_id;

      bs->Read<std::uint8_t>(icon_id);
      bs->Read<float>(markers[icon_id].position.x);
      bs->Read<float>(markers[icon_id].position.y);
      bs->Read<float>(markers[icon_id].position.z);
      bs->Read<std::int8_t>(markers[icon_id].type);
      bs->Read<std::uint32_t>(markers[icon_id].color);
      bs->Read<std::uint8_t>(markers[icon_id].style);

      markers[icon_id].icon_id = icon_id;
      markers[icon_id]._enabled = true;
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrDisableMapIcon) {
      std::uint8_t icon_id;

      bs->Read<std::uint8_t>(icon_id);

      markers[icon_id]._enabled = false;
    }
  }

  std::array<Marker, 100> markers;
};
