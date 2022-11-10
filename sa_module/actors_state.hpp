#pragma once

#include <nlohmann/json.hpp>

#include <CPed.h>
#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "vector.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;
class BitStream;

class actors_state : public Singleton<actors_state> {
public:
  actors_state() = default;
  ~actors_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template<typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->GetActorPool();

    auto& this_pool = to["actor_pool"];
    for (auto i = 0u; i < 1000; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto actor_info = samp_pool->m_pGameObject[i];
      if (!actor_info) continue;

      ActorInfo this_state{};

      this_state.id = i;
      this_state.model = actor_info->m_nModelIndex;
      this_state.position = actor_info->GetPosition();
      this_state.rotation = actor_info->GetHeading();
      this_state.health = actor_info->m_fHealth;
      this_state.invulnerable = actor_info->m_nPhysicalFlags.bBulletProof;

      this_pool.emplace_back(this_state);
    }
  }

  template<typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp_pool = RefNetGame(version)->GetActorPool();
    for (auto& j : from["actor_pool"]) {
      auto this_state = j.get<ActorInfo>();

      using actor_info_t = decltype(get_actor_info_type(version));

      samp_pool->Create(reinterpret_cast<actor_info_t*>(&this_state));

      auto game_ped = samp_pool->Get(this_state.id)->m_pGamePed;
      game_ped->Teleport(this_state.position, true);
      game_ped->m_fCurrentRotation = this_state.rotation;
      game_ped->m_fAimingRotation = this_state.rotation;
      game_ped->SetHeading(this_state.rotation);

      reinterpret_cast<void(__thiscall*)(CEntity*)>(0x446F90)(game_ped);
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
