#pragma once

#include <nlohmann/json_fwd.hpp>

#include "samp_types.hpp"
#include "BitStream.hpp"
#include "sampapi/CObjectEdit.h"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "vector.hpp"

class rpc_handler;

class object_edit_state : public Singleton<object_edit_state> {
public:
  object_edit_state() = default;
  ~object_edit_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template<typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp = RefObjectEdit(version);

    std::optional<ObjectEdit> this_state{};

    if (samp->m_bEnabled) {
      this_state = ObjectEdit{};

      this_state->edit_type = samp->m_nEditType;
      this_state->edit_mode = samp->m_nEditMode;
      this_state->rotation = samp->rotation;
      this_state->process_type = samp->m_nProcessType;

      if (samp->m_nEditType == 2) {
        this_state->object_idx = samp->m_nAttachedObjectIndex;
      }
      else if (samp->m_nEditType == 1) {
        this_state->object_idx = samp->m_nEditObjectId;
      }

      this_state->is_player_object = samp->m_bIsPlayerObject;

      auto matrix = reinterpret_cast<float*>(&samp->m_entityMatrix);
      for (auto i = 0u; i < 4; ++i) {
        for (auto k = 0u; k < 4; ++k) {
          this_state->matrix[i][k] = matrix[i * 4 + k];
        }
      }
    }

    to.emplace("object_edit", this_state);
  }

  template<typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp = RefObjectEdit(version);

    auto this_state = from["object_edit"].get<std::optional<ObjectEdit>>();

    if (this_state.has_value()) {
      samp->m_nEditType = this_state->edit_type;
      samp->m_nEditMode = this_state->edit_type;
      samp->m_nProcessType = this_state->edit_type;
      samp->rotation = this_state->rotation;
      if (this_state->edit_type == 2) {
        samp->m_nAttachedObjectIndex = this_state->object_idx;
      }
      else if (this_state->edit_type == 1) {
        samp->m_nEditObjectId = static_cast<unsigned short>(this_state->object_idx);
      }
      auto matrix = reinterpret_cast<float*>(&samp->m_entityMatrix);
      for (auto i = 0u; i < 4; ++i) {
        for (auto k = 0u; k < 4; ++k) {
          matrix[i * 4 + k] = this_state->matrix[i][k];
        }
      }
      samp->m_bIsPlayerObject = this_state->is_player_object;

      samp->m_nLastSentNotificationTick = GetTickCount();
      samp->m_bEnabled = true;
      samp->m_bRenderScaleButton = this_state->edit_type == 2;
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
