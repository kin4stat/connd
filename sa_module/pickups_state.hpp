#pragma once

#include <nlohmann/json.hpp>
#include <CPickups.h>

#include "samp_types.hpp"
#include "BitStream.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;

class pickups_state : public Singleton<pickups_state> {
public:
  pickups_state() = default;
  ~pickups_state() override = default;

  void init(rpc_handler*) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto& json_pool = to["pickup_pool"];

    auto samp_pool = RefNetGame(version)->GetPickupPool();

    for (auto i = 0u; i < 2048; ++i) {
      if (samp_pool->m_handle[i]) {
        Pickup this_state{};

        this_state._this_id = i;

        if (samp_pool->m_nId[i] == -1) {
          const auto& pickup_info = samp_pool->m_weapon[i];
          if (!pickup_info.m_bExists) continue;
          this_state._this_type = Pickup::kWeapon;
          auto& game_info = CPickups::aPickUps[CPickups::GetActualPickupIndex(samp_pool->m_handle[i])];

          this_state.ex_owner = pickup_info.m_nExOwner;
          this_state.model_id = game_info.m_nModelIndex;
          this_state.ammo = game_info.m_nAmmo;
          this_state.position = game_info.GetPosn();
        }
        else {
          const auto& pickup_info = samp_pool->m_object[i];

          this_state._this_type = Pickup::kDefault;

          this_state.model_id = pickup_info.m_nModel;
          this_state.type = pickup_info.m_nType;
          this_state.position = pickup_info.m_position;
        }

        json_pool.emplace_back(this_state);
      }
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp_pool = RefNetGame(version)->GetPickupPool();
    for (auto& arr : from["pickup_pool"]) {
      auto this_state = arr.get<Pickup>();

      if (this_state._this_type == Pickup::kWeapon) {
        samp_pool->CreateWeapon(this_state.model_id, this_state.position, this_state.ammo, this_state.ex_owner);
      }
      else if (this_state._this_type == Pickup::kDefault) {
        using pickup_t = decltype(get_pickup_type(version));
        samp_pool->Create(reinterpret_cast<pickup_t*>(&this_state), this_state._this_id);
      }
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
  }
};
