#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "sampapi/sampapi.h"
#include "cp2utf.hpp"
#include "sampapi/CNetGame.h"

class rpc_handler;


class labels_state : public Singleton<labels_state> {
public:
  labels_state() = default;
  ~labels_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto& json_pool = to["label_pool"];

    auto pool = RefNetGame(version)->m_pPools->m_pLabel;

    for (auto i = 0u; i < 2048; ++i) {
      if (pool->m_bNotEmpty[i]) {
        auto& samp_label = pool->m_object[i];

        Label temp;

        temp.this_id = i;

        temp.text = cp2utf(samp_label.m_pText);
        temp.color = samp_label.m_color;
        temp.position = samp_label.m_position;
        temp.draw_distance = samp_label.m_fDrawDistance;
        temp.draw_behind_walls = samp_label.m_bBehindWalls;
        temp.attached_to_player = samp_label.m_nAttachedToPlayer;
        temp.attached_to_vehicle = samp_label.m_nAttachedToVehicle;

        json_pool.emplace_back(std::move(temp));
      }
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto label_pool = RefNetGame(version)->m_pPools->m_pLabel;
    for (auto& j : from["label_pool"]) {
      auto temp = j.get<Label>();

      label_pool->Create(temp.this_id,
                         utf2cp(temp.text).c_str(),
                         temp.color,
                         temp.position,
                         temp.draw_distance,
                         temp.draw_behind_walls,
                         temp.attached_to_player,
                         temp.attached_to_vehicle);
    }
  }
};
