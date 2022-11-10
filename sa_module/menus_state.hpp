#pragma once
#include <nlohmann/json.hpp>

#include "samp_types.hpp"
#include "cp2utf.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "rpc_handler.hpp"
#include "sampapi/CNetGame.h"

class menus_state : public Singleton<menus_state> {
public:
  menus_state() = default;
  ~menus_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->GetMenuPool();

    auto& menu_pool = to["menu_pool"];
    auto& items = menu_pool["items"];

    for (auto i = 0u; i < 128; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto info = samp_pool->m_pObject[i];
      if (!info) continue;

      Menu this_state{};

      this_state._this_id = i;

      this_state.title = cp2utf(info->m_szTitle);
      for (auto column = 0u; column < info->m_nColumns; ++column) {
        for (auto row = 0u; row < 12; ++row) {
          if (strlen(info->m_szItems[row][column]))
            this_state.items[column].emplace_back(cp2utf(info->m_szItems[row][column]));
          else
            break;
        }
        this_state.headers.emplace_back(cp2utf(info->m_szHeader[column]));
      }

      this_state.pos_x = info->m_fPosX;
      this_state.pos_y = info->m_fPosY;
      this_state.first_column_width = info->m_fFirstColumnWidth;
      this_state.second_column_width = info->m_fSecondColumnWidth;
      this_state.num_columns = info->m_nColumns;

      this_state.interaction.menu = info->m_interaction.m_bMenu;
      std::ranges::copy(info->m_interaction.m_bRow, std::begin(this_state.interaction.row));
      std::ranges::copy(info->m_interaction.m_bPadding, std::begin(this_state.interaction.padding));
      std::ranges::copy(info->m_nColumnCount, std::begin(this_state.row_count));

      items.emplace_back(std::move(this_state));
    }

    menu_pool.emplace("current_menu", samp_pool->m_nCurrent);
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp_pool = RefNetGame(version)->GetMenuPool();

    auto& menu_pool = from["menu_pool"];
    for (auto& item : menu_pool["items"]) {
      using interaction_t = decltype(get_interaction_type(version));

      auto this_state = item.get<Menu>();

      auto title = utf2cp(this_state.title);

      samp_pool->Create(this_state._this_id,
                        this_state.title.c_str(),
                        this_state.pos_x,
                        this_state.pos_y,
                        this_state.num_columns,
                        this_state.first_column_width,
                        this_state.second_column_width,
                        reinterpret_cast<interaction_t*>(&this_state.interaction));

      auto menu = samp_pool->m_pObject[this_state._this_id];


      std::uint8_t i = 0u;
      for (auto& header : this_state.headers) {
        menu->SetColumnTitle(i, utf2cp(header).c_str());

        std::uint8_t k = 0u;
        for (auto menu_item : this_state.items[i]) {
          menu->AddItem(i, k++, utf2cp(menu_item).c_str());
        }
        ++i;
      }
    }
    samp_pool->Show(menu_pool["current_menu"].get<std::uint8_t>());
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
