#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "vector.hpp"
#include "rpc_handler.hpp"
#include "Utils.hpp"

class rpc_handler;
class BitStream;

class remove_building_state : public Singleton<remove_building_state> {
public:
  remove_building_state() = default;
  ~remove_building_state() override = default;

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_ScrRemoveBuildingForPlayer) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    to.emplace("removed_buildings", std::move(removed_buildings));
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    removed_buildings = from["removed_buildings"].get<std::vector<RemoveBuilding>>();

    for (const auto& removed_building : removed_buildings) {
      BitStream remove_building{};
      remove_building.Write<std::uint32_t>(removed_building.model_id);
      remove_building.Write<float>(removed_building.position.x);
      remove_building.Write<float>(removed_building.position.y);
      remove_building.Write<float>(removed_building.position.z);
      remove_building.Write<float>(removed_building.radius);

      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrRemoveBuildingForPlayer, remove_building);
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    if (rpc_id == RPCEnumeration::RPC_ScrRemoveBuildingForPlayer) {
      RemoveBuilding this_state{};

      bs->Read<std::uint32_t>(this_state.model_id);
      bs->Read<float>(this_state.position.x);
      bs->Read<float>(this_state.position.y);
      bs->Read<float>(this_state.position.z);
      bs->Read<float>(this_state.radius);

      removed_buildings.emplace_back(this_state);
    }
  }

  std::vector<RemoveBuilding> removed_buildings;
};
