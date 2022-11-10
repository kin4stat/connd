#pragma once
#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "rpc_handler.hpp"
#include "sampapi/CObjectSelection.h"

class object_selection_state : public Singleton<object_selection_state> {
public:
  object_selection_state() = default;
  ~object_selection_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    to.emplace("object_selection", RefObjectSelection(version)->m_bIsActive);
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    RefObjectSelection(version)->m_bIsActive = from["object_selection"].get<BOOL>();
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
  }
};
