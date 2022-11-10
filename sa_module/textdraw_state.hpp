#pragma once

#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "cp2utf.hpp"
#include "BitStream.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "sampapi/sampapi.h"
#include "sampapi/CNetGame.h"
#include "sampapi/CTextDrawSelection.h"

class rpc_handler;

class textdraw_state : public Singleton<textdraw_state> {
public:
  textdraw_state() = default;
  ~textdraw_state() override = default;

  void init(rpc_handler*) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->m_pPools->m_pTextdraw;
    auto& this_pool = to["textdraw_pool"];

    for (auto i = 0u; i < 2048 + 256; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto textdraw = samp_pool->m_pObject[i];
      if (!textdraw) continue;

      Textdraw this_state{};

      this_state.box = textdraw->m_data.m_bBox;
      this_state.left = textdraw->m_data.m_bLeft;
      this_state.right = textdraw->m_data.m_bRight;
      this_state.center = textdraw->m_data.m_bCenter;
      this_state.proportional = textdraw->m_data.m_nProportional;
      this_state.letter_width = textdraw->m_data.m_fLetterWidth;
      this_state.letter_height = textdraw->m_data.m_fLetterHeight;
      this_state.letter_color = textdraw->m_data.m_letterColor;
      this_state.box_width = textdraw->m_data.m_fBoxSizeX;
      this_state.box_height = textdraw->m_data.m_fBoxSizeY;
      this_state.box_color = textdraw->m_data.m_boxColor;
      this_state.shadow = textdraw->m_data.m_nShadow;
      this_state.outline = textdraw->m_data.m_nOutline;
      this_state.back_color = textdraw->m_data.m_backgroundColor;
      this_state.style = textdraw->m_data.m_nStyle;
      this_state.x = textdraw->m_data.m_fX;
      this_state.y = textdraw->m_data.m_fY;
      this_state.model = textdraw->m_data.m_nModel;
      this_state.rotation = textdraw->m_data.m_rotation;
      this_state.zoom = textdraw->m_data.m_fZoom;
      this_state.is_selectable = textdraw->m_data.is_selectable;
      std::memcpy(&this_state.color, textdraw->m_data.m_aColor, 4);
      this_state.text = cp2utf(textdraw->m_szText);
      this_state._this_id = i;

      this_pool.emplace_back(std::move(this_state));
    }

    auto selection = RefTextdrawSelection(version);
    to["textdraw_selection"] = TextdrawSelecion{
      selection->m_bIsActive,
      selection->m_hoveredColor
    };
  }

  template <typename T>
  void restore(T version, const nlohmann::json& from) {
    auto samp_pool = RefNetGame(version)->m_pPools->m_pTextdraw;
    for (auto& arr : from["textdraw_pool"]) {
      auto this_state = arr.get<Textdraw>();

      using transmit_t = decltype(get_transmit_type(version));

      samp_pool->Create(this_state._this_id,
                        reinterpret_cast<transmit_t*>(&this_state),
                        utf2cp(this_state.text).c_str());
    }
    auto selection_state = from["textdraw_selection"].get<TextdrawSelecion>();
    auto selection = RefTextdrawSelection(version);

    selection->m_bIsActive = selection_state.active;
    selection->m_hoveredColor = selection_state.hover_color;
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
