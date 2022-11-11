#pragma once

#include <nlohmann/json.hpp>
#include <CPed.h>
#include <CVehicle.h>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "rpc_handler.hpp"
#include "cp2utf.hpp"
#include "Utils.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;
class BitStream;

class remote_players_state : public Singleton<remote_players_state> {
public:
  remote_players_state() = default;
  ~remote_players_state() override = default;

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_ScrAttachObjectToPlayer ||
        rpc == RPCEnumeration::RPC_ScrDestroyObject ||
        rpc == RPCEnumeration::RPC_ScrSetPlayerSkillLevel) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->GetPlayerPool();

    auto& player_pool = to["player_pool"];
    for (auto i = 0u; i < 1004; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto player_info = samp_pool->m_pObject[i];
      if (!player_info) continue;

      Player this_state{};
      this_state._this_id = i;
      this_state.nick = cp2utf(player_info->m_szNick);
      this_state.is_npc = player_info->m_bIsNPC;

      this_state.spawned = true;

      const auto remote_player = player_info->m_pPlayer;

      if (remote_player) {
        this_state.color = remote_player->GetColorAsRGBA();
        this_state.team = remote_player->m_nTeam;

        this_state.show_nametags = remote_player->m_bDrawLabels;
      }

      if (!remote_player ||
        !remote_player->m_pPed ||
        remote_player->m_nState == 0) {
        this_state.spawned = false;
        goto SAVE_;
      }

      this_state.model = remote_player->m_pPed->GetModelIndex();

      if (remote_player->m_nState == sampapi::v037r3::CRemotePlayer::PLAYER_STATE_DRIVER ||
        remote_player->m_nState == sampapi::v037r3::CRemotePlayer::PLAYER_STATE_PASSENGER &&
        remote_player->m_pVehicle) {
        if (remote_player->m_pVehicle->m_pGameVehicle)
          this_state.position = remote_player->m_pVehicle->m_pGameVehicle->GetPosition();
      }
      else {
        this_state.position = remote_player->m_onfootData.m_position;
      }

      for (auto k = 0u; k < skill_levels[i].size(); ++k) {
        if (skill_levels[i][k] != 0)
          this_state.skills.emplace_back(k, skill_levels[i][k]);
      }

      this_state.fightstyle = remote_player->m_pPed->m_pGamePed->m_nFightingStyle;
      for (auto k = 0u; k < 10; k++) {
        if (!remote_player->m_pPed->m_accessories.m_bNotEmpty[k]) continue;

        Accesory this_acc;

        auto& other_acc = remote_player->m_pPed->m_accessories.m_info[k];

        this_acc._this_id = k;
        this_acc.model = other_acc.m_nModel;
        this_acc.bone = other_acc.m_nBone;
        this_acc.offset = other_acc.m_offset;
        this_acc.rotation = other_acc.m_rotation;
        this_acc.scale = other_acc.m_scale;
        this_acc.first_color = other_acc.m_firstMaterialColor;
        this_acc.second_color = other_acc.m_secondMaterialColor;

        this_state.accesories.emplace_back(this_acc);
      }

      if (attached_objects[i].object_id != 0xFFFF) {
        this_state.object_attach = attached_objects[i];
      }

    SAVE_:
      player_pool.emplace_back(this_state);
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto netgame = RefNetGame(version);
    auto player_pool = netgame->GetPlayerPool();

    for (auto& arr : from["player_pool"]) {
      auto this_state = arr.get<Player>();

      BitStream server_join{};
      server_join.Write<std::uint16_t>(this_state._this_id);
      server_join.Write<std::uint32_t>(this_state.color);
      server_join.Write<std::uint8_t>(this_state.is_npc);
      auto nick = utf2cp(this_state.nick);
      server_join.Write<std::uint8_t>(nick.size());
      server_join.Write(nick.c_str(), nick.size());

      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ServerJoin, server_join);

      if (!this_state.spawned) continue;

      BitStream stream_in{};

      stream_in.Write<std::uint16_t>(this_state._this_id);
      stream_in.Write<std::uint8_t>(this_state.team);
      stream_in.Write<std::uint32_t>(this_state.model);
      stream_in.Write<float>(this_state.position.x);
      stream_in.Write<float>(this_state.position.y);
      stream_in.Write<float>(this_state.position.z);
      stream_in.Write<float>(0);
      stream_in.Write<std::uint32_t>(this_state.color);
      stream_in.Write<std::uint8_t>(this_state.fightstyle);

      if (const auto player = player_pool->GetPlayer(this_state._this_id)) {
        player->m_bDrawLabels = this_state.show_nametags;
        player->SetColor(this_state.color);
      }

      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_WorldPlayerAdd, stream_in);

      for (auto& acc : this_state.accesories) {
        BitStream attach_object{};
        attach_object.Write<std::uint16_t>(this_state._this_id);
        attach_object.Write<std::uint32_t>(acc._this_id);
        attach_object.Write<bool>(true);
        attach_object.Write<std::uint32_t>(acc.model);
        attach_object.Write<std::uint32_t>(acc.bone);
        attach_object.Write<float>(acc.offset.x);
        attach_object.Write<float>(acc.offset.y);
        attach_object.Write<float>(acc.offset.z);
        attach_object.Write<float>(acc.rotation.x);
        attach_object.Write<float>(acc.rotation.y);
        attach_object.Write<float>(acc.rotation.z);
        attach_object.Write<float>(acc.scale.x);
        attach_object.Write<float>(acc.scale.y);
        attach_object.Write<float>(acc.scale.z);
        attach_object.Write<std::uint32_t>(acc.first_color);
        attach_object.Write<std::uint32_t>(acc.second_color);

        samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrSetPlayerAttachedObject, attach_object);
      }

      for (const auto& skill : this_state.skills) {
        BitStream set_skill{};
        set_skill.Write<std::uint16_t>(this_state._this_id);
        set_skill.Write<std::uint32_t>(skill.weapon_id);
        set_skill.Write<std::uint16_t>(skill.skill_level);
        samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrSetPlayerSkillLevel, set_skill);

        skill_levels[this_state._this_id][skill.weapon_id] = skill.skill_level;
      }
    }
  }

  template <typename VerTag>
  void restore_phase2(VerTag version, const nlohmann::json& from) {
    for (auto& arr : from["player_pool"]) {
      auto this_state = arr.get<Player>();

      if (this_state.object_attach.has_value()) {
        auto& info = *this_state.object_attach;

        BitStream attach_object{};
        attach_object.Write<std::uint16_t>(this_state._this_id);
        attach_object.Write<std::uint16_t>(info.object_id);
        attach_object.Write<float>(info.offset.x);
        attach_object.Write<float>(info.offset.y);
        attach_object.Write<float>(info.offset.z);
        attach_object.Write<float>(info.rotation.x);
        attach_object.Write<float>(info.rotation.y);
        attach_object.Write<float>(info.rotation.z);

        samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrAttachObjectToPlayer, attach_object);
      }
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    if (rpc_id == RPCEnumeration::RPC_ScrSetPlayerSkillLevel) {
      std::uint16_t for_pid;
      std::uint32_t weapon_id;
      std::uint16_t level;

      bs->Read(for_pid);
      bs->Read(weapon_id);
      bs->Read(level);

      skill_levels[for_pid][weapon_id] = level;
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrAttachObjectToPlayer) {
      std::uint16_t object_id;
      std::uint16_t player_id;
      bs->Read<std::uint16_t>(object_id);
      bs->Read<std::uint16_t>(player_id);

      if (player_id < 1004 && object_id < 2000) {
        auto& info = attached_objects[player_id];
        info.object_id = object_id;
        bs->Read<float>(info.offset.x);
        bs->Read<float>(info.offset.y);
        bs->Read<float>(info.offset.z);
        bs->Read<float>(info.rotation.x);
        bs->Read<float>(info.rotation.y);
        bs->Read<float>(info.rotation.z);
      }
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrDestroyObject) {
      std::uint16_t object_id;
      bs->Read<std::uint16_t>(object_id);
      for (auto& info : attached_objects) {
        if (info.object_id == object_id) {
          info.object_id = 0xFFFF;
        }
      }
    }

    bs->ResetReadPointer();
  }

  std::array<ObjectAttachInfo, 1004> attached_objects;
  std::array<std::array<std::uint16_t, 11>, 1004> skill_levels;
};
