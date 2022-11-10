#pragma once

#include "sampapi/sampapi.h"

template <typename VerTag>
struct Addresses {
};

template <>
struct Addresses<sampapi::v037r1::VersionTag> {
  static constexpr auto dialog_open = 0x6B9C0;
  static constexpr auto dialog_close = 0x6C040;

  static constexpr auto onfoot_sendrate = 0xEC0A8;
  static constexpr auto incar_sendrate = 0xEC0AC;
  static constexpr auto firing_sendrate = 0xEC0B0;
  static constexpr auto sendmult = 0x1048F0;
  static constexpr auto lagcomp_mode = 0x1503F4;

  static constexpr auto operator_new = 0xB4B12;
  static constexpr auto alloc_packet = 0x34800;
  static constexpr auto add_packet_to_producer = 0x375A0;
  static constexpr auto process_network_packet = 0x3B950;

  static constexpr auto get_rpc_id_hook = 0x373A1;
  static constexpr auto rpc_with_params_hook = 0x3743D;
  static constexpr auto rpc_wo_params_hook = 0x373C9;

  static constexpr auto notification_nop = 0x3AE00;
  static constexpr auto received_packets_nop = 0x45B16;
  static constexpr auto reset_nop = 0x46800;
  static constexpr auto clear_remote_system_nop = 0x3CD64;

  static constexpr auto netgame_dctor_hook = 0x9380;
  static constexpr auto quit_hook = 0x64D70;
  static constexpr auto port_change_hook = 0x4FE7C;
  static constexpr auto send_hook = 0x388E0;
  static constexpr auto recv_hook = 0x3CDA0;
  static constexpr auto connect_hook = 0x30640;
  static constexpr auto send_immediate_hook = 0x38E60;
  static constexpr auto process_packet_hook = 0x3B950;
  static constexpr auto send_to_hook = 0x4FFC0;
  static constexpr auto rakclient_factory = 0x33DC0;

  static constexpr auto hostname = 0x21986D;
  static constexpr auto password = 0x21976C;
  static constexpr auto nickname = 0x219A6F;
  static constexpr auto port = 0x21996E;
};

template <>
struct Addresses<sampapi::v037r3::VersionTag> {
  static constexpr auto dialog_open = 0x6F8C0;
  static constexpr auto dialog_close = 0x6FF40;

  static constexpr auto onfoot_sendrate = 0xFE0A8;
  static constexpr auto incar_sendrate = 0xFE0AC;
  static constexpr auto firing_sendrate = 0xFE0B0;
  static constexpr auto sendmult = 0xFE0B4;
  static constexpr auto lagcomp_mode = 0x118970;

  static constexpr auto operator_new = 0xC6B0A;
  static constexpr auto alloc_packet = 0x37BB0;
  static constexpr auto add_packet_to_producer = 0x3A950;
  static constexpr auto process_network_packet = 0x3ED00;

  static constexpr auto get_rpc_id_hook = 0x3A751;
  static constexpr auto rpc_with_params_hook = 0x3A7ED;
  static constexpr auto rpc_wo_params_hook = 0x3A779;

  static constexpr auto notification_nop = 0x3E1B0;
  static constexpr auto received_packets_nop = 0x48EC6;
  static constexpr auto reset_nop = 0x49BB0;
  static constexpr auto clear_remote_system_nop = 0x40114;

  static constexpr auto netgame_dctor_hook = 0x9510;
  static constexpr auto quit_hook = 0x68270;
  static constexpr auto port_change_hook = 0x5322C;
  static constexpr auto send_hook = 0x3BC90;
  static constexpr auto recv_hook = 0x40150;
  static constexpr auto connect_hook = 0x339F0;
  static constexpr auto send_immediate_hook = 0x3C210;
  static constexpr auto process_packet_hook = 0x3ED00;
  static constexpr auto send_to_hook = 0x53370;
  static constexpr auto rakclient_factory = 0x37170;

  static constexpr auto hostname = 0x26DE3D;
  static constexpr auto password = 0x26DD3C;
  static constexpr auto nickname = 0x26E03F;
  static constexpr auto port = 0x26DF3E;
};
