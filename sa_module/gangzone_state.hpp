#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;
class BitStream;


class gangzone_state : public Singleton<gangzone_state> {
public:
  gangzone_state() = default;
  ~gangzone_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) const {
    const auto samp_pool = RefNetGame(version)->m_pPools->m_pGangzone;
    auto& pool = to["gangzone_pool"];

    for (auto i = 0u; i < 1024; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto entry = samp_pool->m_pObject[i];
      if (!entry) continue;

      GangZone this_state{};


      this_state._this_id = i;
      this_state.left = entry->m_rect.left;
      this_state.bottom = entry->m_rect.bottom;
      this_state.right = entry->m_rect.right;
      this_state.top = entry->m_rect.top;
      this_state.color = entry->m_color;
      this_state.alt_color = entry->m_altColor;

      pool.emplace_back(this_state);
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, nlohmann::json& from) const {
    const auto samp_pool = RefNetGame(version)->m_pPools->m_pGangzone;

    for (auto& arr : from["gangzone_pool"]) {
      const auto this_state = arr.get<GangZone>();

      samp_pool->Create(this_state._this_id,
                        this_state.left,
                        this_state.top,
                        this_state.right,
                        this_state.bottom,
                        this_state.color);

      if (this_state.color != this_state.alt_color) {
        samp_pool->StartFlashing(this_state._this_id, this_state.alt_color);
      }
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
