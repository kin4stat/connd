#pragma once

#include <nlohmann/json.hpp>

#include "BitStream.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "samp_types.hpp"
#include "vector.hpp"
#include "sampapi/sampapi.h"
#include "sampapi/CGame.h"

class rpc_handler;


class checkpoint_state : public Singleton<checkpoint_state> {
public:
  checkpoint_state() = default;
  ~checkpoint_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template<typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto game = RefGame(version);

    Checkpoints this_state{};

    if (game->m_checkpoint.m_bEnabled) {
      this_state.checkpoint.emplace(game->m_checkpoint.m_position,
                                    game->m_checkpoint.m_size);
    }
    if (game->m_racingCheckpoint.m_bEnabled) {
      this_state.race_checkpoint.emplace(game->m_racingCheckpoint.m_currentPosition,
                                         game->m_racingCheckpoint.m_nextPosition,
                                         game->m_racingCheckpoint.m_fSize, game->m_racingCheckpoint.m_nType);
    }

    to.emplace("checkpoints", this_state);
  }

  template<typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto game = RefGame(version);

    auto this_state = from["checkpoints"].get<Checkpoints>();

    if (this_state.checkpoint.has_value()) {
      sampapi::CVector pos = this_state.checkpoint->position;
      sampapi::CVector size = this_state.checkpoint->size;
      game->SetCheckpoint(&pos, &size);
      game->m_checkpoint.m_bEnabled = true;
    }
    if (this_state.race_checkpoint.has_value()) {
      sampapi::CVector next_position = this_state.race_checkpoint->next_position;
      sampapi::CVector current_position = this_state.race_checkpoint->current_position;
      game->SetRacingCheckpoint(this_state.race_checkpoint->type,
                                &next_position,
                                &current_position,
                                this_state.race_checkpoint->size);

      game->m_racingCheckpoint.m_bEnabled = true;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
