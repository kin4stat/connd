#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "Utils.hpp"
#include "cp2utf.hpp"
#include "kthook/kthook.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "sampapi/CDialog.h"

class rpc_handler;
class BitStream;

class dialog_state : public Singleton<dialog_state> {
  using CDialog__Show = void(CTHISCALL*)(void*, int, int, const char*, const char*, const char*, const char*, int);
  using CDialog__Close = void(CTHISCALL*)(void*, char);

public:
  dialog_state() = default;
  ~dialog_state() override = default;

  template <typename VerTag>
  void init(WRAP_VERTAG version, rpc_handler* rpch) {

    dialog_show_hook.set_dest(sampapi::GetAddress(Addresses<VerTag>::dialog_open));
    dialog_close_hook.set_dest(sampapi::GetAddress(Addresses<VerTag>::dialog_close));

    dialog_show_hook.set_cb([this](const auto& hook,
                                   void* dialog_ptr,
                                   int id,
                                   int type,
                                   const char* caption,
                                   const char* text,
                                   const char* left_button,
                                   const char* right_button,
                                   int serverside) {
      Dialog temp;
      temp.id = id;
      temp.type = type;
      temp.caption = cp2utf(caption);
      temp.text = cp2utf(text);
      temp.left_button = cp2utf(left_button);
      temp.right_button = cp2utf(right_button);
      temp.serverside = serverside;

      this_state.emplace(std::move(temp));

      return hook.get_trampoline()(dialog_ptr, id, type, caption, text, left_button, right_button, serverside);
    });
    dialog_close_hook.set_cb([this](const auto& hook, void* dialog_ptr, char btn) {
      this_state.reset();

      return hook.get_trampoline()(dialog_ptr, btn);
    });

    dialog_show_hook.install();
    dialog_close_hook.install();
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    to.emplace("dialog", std::move(this_state));
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    this_state = from["dialog"].get<std::optional<Dialog>>();

    if (this_state.has_value())
      RefDialog(version)->Show(this_state->id,
                               this_state->type,
                               utf2cp(this_state->caption).c_str(),
                               utf2cp(this_state->text).c_str(),
                               utf2cp(this_state->left_button).c_str(),
                               utf2cp(this_state->right_button).c_str(),
                               this_state->serverside);
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }

  kthook::kthook_simple<CDialog__Show> dialog_show_hook;
  kthook::kthook_simple<CDialog__Close> dialog_close_hook;

  std::optional<Dialog> this_state{};
};
