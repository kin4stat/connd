#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "raknet_state.hpp"
#include "Utils.hpp"
#include "RakNet.hpp"
#include <CWeather.h>
#include <CClock.h>
#include "crtp_singleton.hpp"
#include "sampapi/sampapi.h"
#include "sampapi/CNetGame.h"

class rpc_handler;

class netgame_state : public Singleton<netgame_state> {
public:
  netgame_state() = default;
  ~netgame_state() override = default;

  void init(rpc_handler*) {
    is_server_tick = new char{};

    if (utils::get_samp_version() == utils::samp_version::kR1) {
      utils::SetRaw(sampapi::GetAddress(0x9CCC5),
                    "\xC6\x05",
                    2);
      utils::SetRaw(sampapi::GetAddress(0x9CCC5 + 2),
                    reinterpret_cast<const char*>(&is_server_tick),
                    4);
      utils::SetRaw(sampapi::GetAddress(0x9CCC5 + 6),
                    "\x01",
                    1);
      utils::SetRaw(sampapi::GetAddress(0x9CCC5 + 7),
                    "\xC2\x04\x00",
                    3);
    }
    else {
      utils::SetRaw(sampapi::GetAddress(0xA1035 + 2),
                    reinterpret_cast<const char*>(&is_server_tick),
                    4);
    }
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    const auto netgame = RefNetGame(version);
    const auto settings = netgame->m_pSettings;

    NetGameSettings this_state{};

    this_state.m_bUseCJWalk = settings->m_bUseCJWalk;
    this_state.m_nDeadDropsMoney = settings->m_nDeadDropsMoney;
    this_state.m_bAllowWeapons = settings->m_bAllowWeapons;
    this_state.m_fGravity = settings->m_fGravity;
    this_state.m_bEnterExit = settings->m_bEnterExit;
    this_state.m_bVehicleFriendlyFire = settings->m_bVehicleFriendlyFire;
    this_state.m_bHoldTime = settings->m_bHoldTime;
    this_state.m_bInstagib = settings->m_bInstagib;
    this_state.m_bZoneNames = settings->m_bZoneNames;
    this_state.m_bFriendlyFire = settings->m_bFriendlyFire;
    this_state.m_bClassesAvailable = settings->m_bClassesAvailable;
    this_state.m_fNameTagsDrawDist = settings->m_fNameTagsDrawDist;
    this_state.m_bManualVehicleEngineAndLight = settings->m_bManualVehicleEngineAndLight;
    this_state.m_nWorldTimeHour = settings->m_nWorldTimeHour;
    this_state.m_nWorldTimeMinute = settings->m_nWorldTimeMinute;
    this_state.m_nWeather = settings->m_nWeather;
    this_state.m_bNoNametagsBehindWalls = settings->m_bNoNametagsBehindWalls;
    this_state.m_nPlayerMarkersMode = settings->m_nPlayerMarkersMode;
    this_state.m_fChatRadius = settings->m_fChatRadius;
    this_state.m_bNameTags = settings->m_bNameTags;
    this_state.m_bLtdChatRadius = settings->m_bLtdChatRadius;
    this_state.update_camera_target = netgame->m_bUpdateCameraTarget;

    this_state.disable_collision = netgame->m_bDisableCollision;
    this_state.stunt_bonus = *reinterpret_cast<unsigned*>(0xA4A474);
    this_state.lan_mode = netgame->m_bLanMode;
    this_state.playerid = netgame->GetPlayerPool()->m_localInfo.m_nId;

    this_state.timer_ticks = 0;
    {
      using namespace std::chrono;

      this_state.save_tick = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    auto read_unsigned = [](std::uintptr_t address) {
      return *reinterpret_cast<unsigned*>(sampapi::GetAddress(address));
    };

    this_state.onfoot_sendrate = read_unsigned(Addresses<VerTag>::onfoot_sendrate);
    this_state.incar_sendrate = read_unsigned(Addresses<VerTag>::incar_sendrate);
    this_state.firing_sendrate = read_unsigned(Addresses<VerTag>::firing_sendrate);
    this_state.sendmult = read_unsigned(Addresses<VerTag>::sendmult);
    this_state.lagcomp_mode = read_unsigned(Addresses<VerTag>::lagcomp_mode);
    if (*is_server_tick) {
      this_state.timer_ticks = CTimer::m_snTimeInMilliseconds;
    }

    if (!this_state.lagcomp_mode) {
      this_state.lagcomp_mode = 0;
    }

    std::ranges::copy(settings->m_fWorldBoundaries, std::begin(this_state.m_fWorldBoundaries));

    to.emplace("netgame_settings", this_state);
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    const auto netgame = RefNetGame(version);
    const auto settings = netgame->m_pSettings;

    const auto this_state = from["netgame_settings"].get<NetGameSettings>();

    netgame->m_bDisableCollision = this_state.disable_collision;
    netgame->m_bUpdateCameraTarget = this_state.update_camera_target;

    settings->m_nWorldTimeHour = this_state.m_nWorldTimeHour;
    settings->m_nWorldTimeMinute = this_state.m_nWorldTimeMinute;
    settings->m_nWeather = this_state.m_nWeather;

    if (this_state.timer_ticks != 0) {
      using namespace std::chrono;

      auto new_tick = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();

      CTimer::m_snTimeInMilliseconds = this_state.timer_ticks + (new_tick - this_state.save_tick);
    }

    BitStream set_world_boundaries{};

    set_world_boundaries.Write<float>(this_state.m_fWorldBoundaries[0]);
    set_world_boundaries.Write<float>(this_state.m_fWorldBoundaries[1]);
    set_world_boundaries.Write<float>(this_state.m_fWorldBoundaries[2]);
    set_world_boundaries.Write<float>(this_state.m_fWorldBoundaries[3]);

    samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrSetWorldBounds, set_world_boundaries);

    CWeather::ForceWeather(this_state.m_nWeather);
    CClock::ms_nGameClockHours = this_state.m_nWorldTimeHour;
    CClock::ms_nGameClockMinutes = this_state.m_nWorldTimeMinute;
    CClock::NormaliseGameClock();


    BitStream out_bs{};

    out_bs.Write(this_state.m_bZoneNames);
    out_bs.Write(this_state.m_bUseCJWalk);
    out_bs.Write(this_state.m_bAllowWeapons);
    out_bs.Write(this_state.m_bLtdChatRadius);
    out_bs.Write(this_state.m_fChatRadius);
    out_bs.Write(this_state.stunt_bonus);
    out_bs.Write(this_state.m_fNameTagsDrawDist);
    out_bs.Write(this_state.m_bEnterExit);
    out_bs.Write(this_state.m_bNoNametagsBehindWalls);
    out_bs.Write(this_state.m_bManualVehicleEngineAndLight);
    out_bs.Write(this_state.m_bClassesAvailable);
    out_bs.Write(this_state.playerid);
    out_bs.Write(this_state.m_bNameTags);
    out_bs.Write(this_state.m_nPlayerMarkersMode);
    out_bs.Write(this_state.m_nWorldTimeHour);
    out_bs.Write(this_state.m_nWeather);
    out_bs.Write(this_state.m_fGravity);
    out_bs.Write(this_state.lan_mode);
    out_bs.Write(this_state.m_nDeadDropsMoney);
    out_bs.Write(this_state.m_bInstagib);
    out_bs.Write(this_state.onfoot_sendrate);

    out_bs.Write(this_state.incar_sendrate);
    out_bs.Write(this_state.firing_sendrate);
    out_bs.Write(this_state.sendmult);
    out_bs.Write(this_state.lagcomp_mode);

    out_bs.Write<unsigned char>(rak_state.hostname.size());
    out_bs.Write(rak_state.hostname.c_str(), rak_state.hostname.size());

    out_bs.Write(this_state.m_bVehicleFriendlyFire);

    samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_InitGame, out_bs);
  }

private:
  char* is_server_tick{};
};
