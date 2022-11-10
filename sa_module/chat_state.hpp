#pragma once

#include <nlohmann/json.hpp>

#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "cp2utf.hpp"
#include "samp_types.hpp"
#include "sampapi/CChat.h"

class rpc_handler;
class BitStream;


class chat_state : public Singleton<chat_state> {
public:
  chat_state() = default;
  ~chat_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_chat = RefChat(version);

    Chat this_state{};
    this_state.mode = samp_chat->m_nMode;

    for (auto& entry : samp_chat->m_entry) {
      if (!entry.m_timestamp) continue;

      Chat::ChatEntry this_entry;

      this_entry.timestamp = entry.m_timestamp;
      this_entry.prefix = cp2utf(entry.m_szPrefix);
      this_entry.text = cp2utf(entry.m_szText);
      this_entry.type = entry.m_nType;
      this_entry.text_color = entry.m_textColor;
      this_entry.prefix_color = entry.m_prefixColor;

      this_state.entries.emplace_back(std::move(this_entry));
    }

    to.emplace("chat", std::move(this_state));
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto this_state = from["chat"].get<Chat>();
    auto samp_chat = RefChat(version);

    samp_chat->m_nMode = this_state.mode;

    memset(samp_chat->m_entry, 0, sizeof(samp_chat->m_entry));

    for (auto& this_entry : this_state.entries) {
      auto prefix = utf2cp(this_entry.prefix);

      samp_chat->AddEntry(this_entry.type,
                          utf2cp(this_entry.text).c_str(),
                          prefix.empty() ? nullptr : prefix.c_str(),
                          this_entry.text_color,
                          this_entry.prefix_color);
    }
    samp_chat->m_bRedraw = true;
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
