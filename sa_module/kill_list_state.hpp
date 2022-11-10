#pragma once

#include <array>
#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "cp2utf.hpp"
#include "sampapi/CDeathWindow.h"

class rpc_handler;
class BitStream;


class kill_list_state : public Singleton<kill_list_state> {
public:
  kill_list_state() = default;
  ~kill_list_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto kill_list = RefDeathWindow(version);

    KillList this_state{};
    this_state.enabled = kill_list->m_bEnabled;

    auto i = 0u;
    for (auto& entry : kill_list->m_entry) {
      auto& this_entry = this_state.entries[i];

      this_entry.killer = cp2utf(entry.m_szKiller);
      this_entry.victim = cp2utf(entry.m_szVictim);
      this_entry.killer_color = entry.m_killerColor;
      this_entry.victim_color = entry.m_victimColor;
      this_entry.weapon = entry.m_nWeapon;
      ++i;
    }

    to.emplace("kill_list", std::move(this_state));
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    const auto this_state = from["kill_list"].get<KillList>();
    const auto kill_list = RefDeathWindow(version);

    kill_list->m_bEnabled = this_state.enabled;
    auto i = 0u;
    for (auto& this_entry : this_state.entries) {
      auto& samp_entry = kill_list->m_entry[i];

      std::ranges::copy(utf2cp(this_entry.killer),
                        std::begin(samp_entry.m_szKiller));
      std::ranges::copy(utf2cp(this_entry.victim),
                        std::begin(samp_entry.m_szVictim));
      samp_entry.m_killerColor = this_entry.killer_color;
      samp_entry.m_victimColor = this_entry.victim_color;
      samp_entry.m_nWeapon = this_entry.weapon;
      ++i;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
