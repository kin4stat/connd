#pragma once

#include <nlohmann/json_fwd.hpp>
#include <CPlayerInfo.h>

#include "samp_types.hpp"

#include "BitStream.hpp"
#include "common.h"
#include "cp2utf.hpp"
#include "rpc_handler.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "Utils.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;


class local_player_state : public Singleton<local_player_state> {
public:
  local_player_state() = default;
  ~local_player_state() override = default;

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_ScrSetPlayerSkillLevel ||
        rpc == RPCEnumeration::RPC_ScrAttachObjectToPlayer ||
        rpc == RPCEnumeration::RPC_ScrDestroyObject) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    LocalPlayer this_state;

    const auto player_pool = RefNetGame(version)->GetPlayerPool();
    const auto local_info = player_pool->GetLocalPlayer();
    const auto& spawn_info = local_info->m_spawnInfo;
    const auto player_ped = FindPlayerPed();

    if (!player_ped) return;

    this_state._this_id = player_pool->m_localInfo.m_nId;
    this_state.position = player_ped->GetPosition();
    RwMatrix rwmat;
    player_ped->GetMatrix()->CopyToRwMatrix(&rwmat);
    CQuaternion quat;
    quat.Set(rwmat);

    this_state.interior_id = local_info->m_nCurrentInterior;
    this_state.rotation = quat;
    this_state.heading = player_ped->GetHeading();

    this_state.health = player_ped->m_fHealth;
    this_state.armour = player_ped->m_fArmour;
    this_state.max_health = player_ped->m_fMaxHealth;
    this_state.armed_weapon = player_ped->m_nActiveWeaponSlot;
    for (auto i = 0u; i < this_state.weapons.size(); ++i) {
      this_state.weapons[i].type = player_ped->m_aWeapons[i].m_nType;
      this_state.weapons[i].total_ammo = player_ped->m_aWeapons[i].m_nTotalAmmo;
    }
    this_state.wanted_level = *reinterpret_cast<char*>(0x58DB60);

    this_state.goto_class_selection = local_info->m_classSelection.m_bEnableAfterDeath;
    this_state.class_selection_number = local_info->m_classSelection.m_nSelected;
    this_state.in_class_selection = local_info->m_classSelection.m_bIsActive;

    if (player_ped->m_pEnex) {
      auto shopname = static_cast<char*>(player_ped->m_pEnex);

      this_state.shop_name = shopname;
    }

    this_state.is_controllable = !player_ped->GetPadFromPlayer()->bPlayerSafe;
    this_state.money = *reinterpret_cast<std::int32_t*>(0xB7CE50);
    this_state.drunk_level = local_info->GetPed()->GetDrunkLevel();
    this_state.team = local_info->m_nTeam;
    this_state.color = local_info->GetColorAsRGBA();
    this_state.model_id = local_info->GetPed()->GetModelIndex();
    this_state.nick = player_pool->m_localInfo.m_szName;

    for (auto i = 0u; i < skill_level.size(); ++i) {
      if (skill_level[i] != 0)
        this_state.skills.emplace_back(i, skill_level[i]);
    }

    this_state.spawn_info.team = spawn_info.m_nTeam;
    this_state.spawn_info.model_id = spawn_info.m_nSkin;
    this_state.spawn_info.field_c = spawn_info.field_c;
    this_state.spawn_info.position = spawn_info.m_position;
    this_state.spawn_info.rotation = spawn_info.m_fRotation;
    std::ranges::copy(spawn_info.m_aAmmo, std::begin(this_state.spawn_info.ammo));
    std::ranges::copy(spawn_info.m_aWeapon, std::begin(this_state.spawn_info.weapon));

    if (object_attach_info.object_id != 0xFFFF) {
      this_state.object_attach = object_attach_info;
    }

    for (auto k = 0u; k < 10; k++) {
      if (!local_info->GetPed()->m_accessories.m_bNotEmpty[k]) continue;

      Accesory this_acc;

      auto& other_acc = local_info->GetPed()->m_accessories.m_info[k];

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

    this_state.is_spectating = local_info->m_bDoesSpectating;
    if (local_info->m_bDoesSpectating) {
      if (local_info->m_spectating.m_nMode) {
        LocalPlayer::SpectateInfo spectate_info;
        spectate_info.object_id = local_info->m_spectating.m_nObject;
        spectate_info.mode = local_info->m_spectating.m_nMode;
        spectate_info.type = local_info->m_spectating.m_nType;
        this_state.spectate_info.emplace(spectate_info);
      }
    }

    to.emplace("local_player_state", this_state);
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto this_state = from["local_player_state"].get<LocalPlayer>();

    auto player_pool = RefNetGame(version)->GetPlayerPool();
    auto local_info = player_pool->GetLocalPlayer();
    auto& spawn_info = local_info->m_spawnInfo;

    // avoid using move operators
    auto nick = utf2cp(this_state.nick);
    player_pool->m_localInfo.m_szName = nick;
    player_pool->m_localInfo.m_nId = this_state._this_id;

    spawn_info.m_position = this_state.position;
    spawn_info.m_fRotation = this_state.heading;
    spawn_info.m_nTeam = this_state.team;
    spawn_info.m_nSkin = this_state.model_id;

    local_info->m_bHasSpawnInfo = true;
    local_info->Spawn();

    local_info->GetPed()->LinkToInterior(this_state.interior_id, true);

    const auto player_ped = FindPlayerPed();
    const auto game_player_info = player_ped->GetPlayerInfoForThisPlayerPed();

    player_ped->Teleport(this_state.position, true);
    player_ped->m_fCurrentRotation = this_state.heading;
    player_ped->m_fAimingRotation = this_state.heading;
    player_ped->SetHeading(this_state.heading);
    reinterpret_cast<void(__thiscall*)(CEntity*)>(0x446F90)(player_ped);

    player_ped->m_nPedFlags.bIsStanding = true;
    player_ped->m_nPedFlags.bWasStanding = true;

    spawn_info.m_position = this_state.spawn_info.position;
    spawn_info.m_fRotation = this_state.spawn_info.rotation;
    spawn_info.m_nTeam = this_state.spawn_info.team;
    spawn_info.m_nSkin = this_state.spawn_info.model_id;
    std::ranges::copy(this_state.spawn_info.ammo, std::begin(spawn_info.m_aAmmo));
    std::ranges::copy(this_state.spawn_info.weapon, std::begin(spawn_info.m_aWeapon));

    player_ped->m_fHealth = this_state.health;
    player_ped->m_fMaxHealth = this_state.max_health;
    player_ped->m_fArmour = this_state.armour;

    *reinterpret_cast<std::uint8_t*>(0x58DB60) = this_state.wanted_level;

    game_player_info->m_nMoney = this_state.money;
    game_player_info->m_nDisplayMoney = this_state.money;
    local_info->m_pPed->SetControllable(this_state.is_controllable);
    local_info->SetColor(this_state.color);

    for (auto& weapon : this_state.weapons) {
      BitStream give_weapon{};
      give_weapon.Write<std::uint32_t>(weapon.type);
      give_weapon.Write<std::uint32_t>(weapon.total_ammo);
      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrGivePlayerWeapon, give_weapon);
    }

    player_ped->SetCurrentWeapon(this_state.armed_weapon);

    for (const auto& skill : this_state.skills) {
      BitStream set_skill{};
      set_skill.Write<std::uint16_t>(this_state._this_id);
      set_skill.Write<std::uint32_t>(skill.weapon_id);
      set_skill.Write<std::uint16_t>(skill.skill_level);
      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrSetPlayerSkillLevel, set_skill);

      skill_level[skill.weapon_id] = skill.skill_level;
    }

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
  }

  template <typename VerTag>
  void restore_phase2(WRAP_VERTAG version, const nlohmann::json& from) {
    auto this_state = from["local_player_state"].get<LocalPlayer>();

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
    }

    const auto player_pool = RefNetGame(version)->GetPlayerPool();
    const auto local_info = player_pool->GetLocalPlayer();
    const auto player_ped = FindPlayerPed();

    if (this_state.in_class_selection) {
      local_info->PrepareForClassSelection();
      local_info->m_classSelection.m_bIsActive = true;
      local_info->m_classSelection.m_nSelected = this_state.class_selection_number;
    }
    local_info->m_classSelection.m_bEnableAfterDeath = this_state.goto_class_selection;

    if (this_state.shop_name.has_value()) {

      // pointer will be lost forever :(
      auto shopname = new char[0x20];
      std::memcpy(shopname, this_state.shop_name->c_str(), this_state.shop_name->size());
      shopname[this_state.shop_name->size()] = '\0';

      player_ped->m_pEnex = shopname;

      local_info->m_pPed->LoadShoppingDataSubsection(shopname);
    }

    if (this_state.is_spectating) {
      local_info->EnableSpectating(true);
    }

    if (this_state.spectate_info.has_value()) {
      if (this_state.spectate_info->type == 1) {
        local_info->SpectateForPlayer(this_state.spectate_info->object_id);
      }
      else if (this_state.spectate_info->type == 2) {
        local_info->SpectateForVehicle(this_state.spectate_info->object_id);
      }
      local_info->m_spectating.m_nMode = this_state.spectate_info->mode;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    if (rpc_id == RPCEnumeration::RPC_ScrSetPlayerSkillLevel) {
      std::uint16_t for_pid;

      bs->Read(for_pid);
      if (for_pid == local_pid) {
        std::uint32_t weapon_id;
        std::uint16_t level;

        bs->Read(weapon_id);
        bs->Read(level);

        skill_level[weapon_id] = level;
      }
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrAttachObjectToPlayer) {
      std::uint16_t object_id;
      std::uint16_t player_id;
      bs->Read<std::uint16_t>(object_id);
      bs->Read<std::uint16_t>(player_id);

      if (player_id == local_pid) {
        auto& info = object_attach_info;
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
      if (object_attach_info.object_id == object_id) {
        object_attach_info.object_id = 0xFFFF;
      }
    }

    bs->ResetReadPointer();
  }

  std::uint16_t local_pid;

  std::array<std::uint16_t, 11> skill_level;
  ObjectAttachInfo object_attach_info;
};
