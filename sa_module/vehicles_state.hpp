#pragma once

#include <nlohmann/json_fwd.hpp>

#include "samp_types.hpp"

#include <CModelInfo.h>
#include "BitStream.hpp"
#include "common.h"
#include "cp2utf.hpp"
#include "rpc_handler.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "Utils.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;


class vehicles_state : public Singleton<vehicles_state> {
public:
  vehicles_state() = default;
  ~vehicles_state() override = default;

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_WorldVehicleAdd ||
        rpc == RPCEnumeration::RPC_ScmEvent ||
        rpc == RPCEnumeration::RPC_ScrRemoveComponent ||
        rpc == RPCEnumeration::RPC_WorldVehicleRemove) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->GetVehiclePool();
    auto player_pool = RefNetGame(version)->GetPlayerPool();
    auto local_ped = FindPlayerPed();
    auto local_pid = player_pool->m_localInfo.m_nId;

    auto& pool = to["vehicle_pool"];
    for (auto i = 0u; i < 2000; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto game_vehicle = samp_pool->m_pGameObject[i];
      auto samp_vehicle = samp_pool->m_pObject[i];
      if (!game_vehicle || !samp_vehicle) continue;

      Vehicle this_state{};

      this_state.driver = 0xFFFF;
      this_state.passengers.fill(0xFFFF);
      this_state.trailer_id = 0xFFFF;

      this_state._this_id = i;
      this_state.model_id = samp_vehicle->GetModelIndex();
      this_state.spawn_pos = samp_pool->m_spawnedAt[i];
      this_state.pos = game_vehicle->GetPosition();

      RwMatrix rwmat;
      game_vehicle->GetMatrix()->CopyToRwMatrix(&rwmat);
      CQuaternion quat;
      quat.Set(rwmat);

      this_state.rotation = quat;
      this_state.int_color1 = game_vehicle->m_nPrimaryColor;
      this_state.int_color2 = game_vehicle->m_nPrimaryColor;
      this_state.color1 = game_vehicle->m_nPrimaryColor;
      this_state.color2 = game_vehicle->m_nPrimaryColor;

      this_state.health = game_vehicle->m_fHealth;
      this_state.interior_id = game_vehicle->m_nAreaCode;
      this_state.siren = samp_vehicle->HasSiren();
      this_state.doors_damage = samp_vehicle->GetDoorsDamage();
      this_state.panel_damage = samp_vehicle->GetPanelsDamage();
      this_state.lights_damage = samp_vehicle->GetLightsDamage();
      this_state.tires_damage = samp_vehicle->GetTires();
      this_state.lock_doors = samp_vehicle->m_bIsLocked;
      this_state.engine_on = samp_vehicle->m_bEngineOn || game_vehicle->m_nVehicleFlags.bEngineOn;
      this_state.lights_on = samp_vehicle->m_bIsLightsOn || game_vehicle->m_nVehicleFlags.bLightsOn;
      this_state.turn_speed = game_vehicle->m_vecTurnSpeed;
      this_state.move_speed = game_vehicle->m_vecMoveSpeed;

      if (game_vehicle->m_pDriver == local_ped) {
        this_state.driver = local_pid;
      }
      auto j = 0u;
      for (auto passenger : game_vehicle->m_apPassengers) {
        if (passenger == local_ped) {
          this_state.passengers[j] = local_pid;
        }
        else {
          for (auto k = 0u; k < 1004; ++k) {
            if (!player_pool->m_bNotEmpty[k]) continue;
            auto player_info = player_pool->GetPlayer(k);
            if (!player_info) continue;
            auto samp_ped = player_info->m_pPed;
            if (!samp_ped) continue;
            auto game_ped = samp_ped->m_pGamePed;
            if (!game_ped) continue;

            if (passenger == game_ped) {
              this_state.passengers[j] = k;
            }
            else if (game_vehicle->m_pDriver == game_ped) {
              this_state.driver = k;
            }
          }
        }

        j++;
      }

      for (auto k = 0u; k < 2000; ++k) {
        if (samp_vehicle->m_pTrailer == samp_pool->m_pObject[k]) {
          this_state.trailer_id = k;
          break;
        }
      }

      auto paintjob_idx = game_vehicle->m_nRemapTxd;
      auto vehicle_info = reinterpret_cast<CVehicleModelInfo*>(CModelInfo::GetModelInfo(this_state.model_id));

      auto k = 0u;
      for (auto remap_idx : vehicle_info->m_anRemapTxds) {
        if (remap_idx == paintjob_idx) {
          this_state.paintjob = k;
          break;
        }
        ++k;
      }

      this_state.number_plate = cp2utf(samp_vehicle->m_szLicensePlateText);
      this_state.mod_slots = mod_slots[i];

      pool.emplace_back(std::move(this_state));
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp_pool = RefNetGame(version)->GetVehiclePool();
    auto player_pool = RefNetGame(version)->GetPlayerPool();
    auto local_ped = FindPlayerPed();
    auto local_pid = player_pool->m_localInfo.m_nId;

    for (auto& arr : from["vehicle_pool"]) {
      auto this_state = arr.get<Vehicle>();

      BitStream stream_in{};

      stream_in.Write<std::uint16_t>(this_state._this_id);
      stream_in.Write<std::uint32_t>(this_state.model_id);
      stream_in.Write<float>(this_state.pos.x);
      stream_in.Write<float>(this_state.pos.y);
      stream_in.Write<float>(this_state.pos.z);
      stream_in.Write<float>(0);
      stream_in.Write<std::uint8_t>(this_state.int_color1);
      stream_in.Write<std::uint8_t>(this_state.int_color2);
      stream_in.Write<float>(this_state.health);
      stream_in.Write<std::uint8_t>(this_state.interior_id);
      stream_in.Write<std::uint32_t>(this_state.doors_damage);
      stream_in.Write<std::uint32_t>(this_state.panel_damage);
      stream_in.Write<std::uint8_t>(this_state.lights_damage);
      stream_in.Write<std::uint8_t>(this_state.tires_damage);
      stream_in.Write<std::uint8_t>(this_state.siren);

      for (auto mod_slot : this_state.mod_slots) {
        stream_in.Write<std::uint8_t>(mod_slot);

        mod_slots[this_state._this_id].emplace_back(mod_slot);
      }

      for (auto i = 0u; i < 14 - this_state.mod_slots.size(); ++i) {
        stream_in.Write<std::uint8_t>(0);
      }

      stream_in.Write<std::uint8_t>(this_state.paintjob);
      stream_in.Write<std::uint32_t>(this_state.color1);
      stream_in.Write<std::uint32_t>(this_state.color2);

      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_WorldVehicleAdd, stream_in);

      samp_pool->m_spawnedAt[this_state._this_id] = this_state.spawn_pos;

      if (this_state.trailer_id != 0xFFFF) {
        BitStream trailer_attach{};
        trailer_attach.Write<std::uint16_t>(this_state.trailer_id);
        trailer_attach.Write<std::uint16_t>(this_state._this_id);

        samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrAttachTrailerToVehicle, trailer_attach);
      }

      if (this_state.driver != 0xFFFF) {
        auto samp_ped = player_pool->GetLocalPlayer()->GetPed();
        if (this_state.driver != local_pid) {
          samp_ped = player_pool->GetPlayer(this_state.driver)->m_pPed;
        }

        if (samp_ped)
          samp_ped->PutIntoVehicle(samp_pool->GetRef(this_state._this_id), 0);
      }

      for (auto k = 0u; k < this_state.passengers.size(); ++k) {
        if (this_state.passengers[k] != 0xFFFF) {
          auto samp_ped = player_pool->GetLocalPlayer()->GetPed();
          if (this_state.passengers[k] != local_pid) {
            samp_ped = player_pool->GetPlayer(this_state.passengers[k])->m_pPed;
          }

          if (samp_ped)
            samp_ped->PutIntoVehicle(samp_pool->GetRef(this_state._this_id), k + 1);
        }
      }

      auto game_vehicle = samp_pool->m_pGameObject[this_state._this_id];
      auto samp_vehicle = samp_pool->m_pObject[this_state._this_id];

      game_vehicle->GetMatrix()->SetRotate(this_state.rotation);
      samp_vehicle->m_bEngineOn = this_state.engine_on;
      samp_vehicle->m_bIsLightsOn = this_state.lights_on;
      game_vehicle->m_nVehicleFlags.bEngineOn = this_state.engine_on;
      game_vehicle->m_nVehicleFlags.bLightsOn = this_state.lights_on;
      game_vehicle->m_vecTurnSpeed = this_state.turn_speed;
      game_vehicle->m_vecMoveSpeed = this_state.move_speed;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    if (rpc_id == RPCEnumeration::RPC_WorldVehicleAdd) {
      std::uint16_t vehicle_id;

      bs->Read<std::uint16_t>(vehicle_id);
      bs->IgnoreBits(304);
      for (auto i = 0u; i < 14; ++i) {
        std::uint8_t mod_slot;
        bs->Read(mod_slot);

        if (mod_slot != 0) {
          mod_slots[vehicle_id].emplace_back(mod_slot);
        }
      }
    }
    else if (rpc_id == RPCEnumeration::RPC_ScmEvent) {
      std::uint16_t player_id;
      std::uint32_t event;
      std::uint32_t vehicle_id;
      std::uint32_t param1;
      std::uint32_t param2;

      bs->Read<std::uint16_t>(player_id);
      bs->Read<std::uint32_t>(event);
      bs->Read<std::uint32_t>(vehicle_id);
      bs->Read<std::uint32_t>(param1);
      bs->Read<std::uint32_t>(param2);

      if (event == 2) {
        mod_slots[vehicle_id].emplace_back(static_cast<std::uint8_t>(param1 - 999));
      }
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrRemoveComponent) {
      std::uint16_t vehicle_id;
      std::uint16_t component_id;

      bs->Read<std::uint16_t>(vehicle_id);
      bs->Read<std::uint16_t>(component_id);

      std::erase_if(mod_slots[vehicle_id], [component_id](auto v) {
        return v == (component_id - 999);
      });
    }
    else if (rpc_id == RPCEnumeration::RPC_WorldVehicleRemove) {
      std::uint16_t vehicle_id;

      bs->Read<std::uint16_t>(vehicle_id);

      mod_slots[vehicle_id].clear();
    }

    bs->ResetReadPointer();
  }

  std::array<std::vector<std::uint8_t>, 2000> mod_slots;
};
