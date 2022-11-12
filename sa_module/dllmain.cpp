#define MAGIC_ENUM_RANGE_MIN -256
#define MAGIC_ENUM_RANGE_MAX 256

#include <WinSock2.h>
#include "logger.hpp"
#include "BitStream.hpp"
#include "kthook/kthook.hpp"
#include "sampapi/sampapi.h"
#include "sampapi/CInput.h"
#include "magic_enum.hpp"
#include <format>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <chrono>

#include "samp_utils.hpp"
#include "actors_state.hpp"
#include "camera_state.hpp"
#include "chat_state.hpp"
#include "checkpoint_state.hpp"
#include "dialog_state.hpp"
#include "gangzone_state.hpp"
#include "kill_list_state.hpp"
#include "labels_state.hpp"
#include "local_player_state.hpp"
#include "markers_state.hpp"
#include "menus_state.hpp"
#include "net_stats_state.hpp"
#include "netgame_state.hpp"
#include "object_edit_state.hpp"
#include "object_select_state.hpp"
#include "objects_state.hpp"
#include "pickups_state.hpp"
#include "remote_player_state.hpp"
#include "remove_building_state.hpp"
#include "textdraw_state.hpp"
#include "vehicles_state.hpp"


#include "ipc/ipc_client.hpp"
#include "RakNet.hpp"
#include "nlohmann/json.hpp"
#include "raknet_state.hpp"
#include "rpc_handler.hpp"

#include "Utils.hpp"

#include "ipc/ipc_server.hpp"
#include "sampapi/CNetGame.h"

enum class EmulState {
  kNone,
  kWaitingForFirstRequest,
  kCanEmulCookie,
  kWaitingForSecondRequest,
  kCanEmulReply,
  kWaitingForThirdRequest,
  kCanEmulAuthKey,
  kWaitingForAuthKey,
  kCanEmulConnectionAccept,
  kWaitingForClientJoin,
  kCanEmulStaticData,
};

struct settings {
  std::uint32_t default_timeout;
  std::uint32_t quit_timeout;
  std::uint16_t port;
} g_settings;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(settings, default_timeout, quit_timeout)

bool ignore_next_simm;
nlohmann::json samp_state;
std::atomic<EmulState> current_state = EmulState::kNone;
ipc_client client;
bool init_once{true};
bool set_filter_once{true};
bool set_port{false};
std::uintptr_t htons_jmp_back{0};
unsigned short port{0};

template <typename VerTag>
StartupArgs get_startup_args(WRAP_VERTAG version) {
  return {
    reinterpret_cast<char*>(sampapi::GetAddress(Addresses<VerTag>::nickname)),
    reinterpret_cast<char*>(sampapi::GetAddress(Addresses<VerTag>::hostname)),
    reinterpret_cast<char*>(sampapi::GetAddress(Addresses<VerTag>::port)),
    reinterpret_cast<char*>(sampapi::GetAddress(Addresses<VerTag>::password)),
  };
}

bool can_restore() {
  if (utils::get_samp_version() == utils::samp_version::kR1) {
    auto ver_tag = sampapi::v037r1::VersionTag{};

    return samp_state["startup_args"].get<StartupArgs>() == get_startup_args(ver_tag);
  }
  else if (utils::get_samp_version() == utils::samp_version::kR3) {
    auto ver_tag = sampapi::v037r3::VersionTag{};

    return samp_state["startup_args"].get<StartupArgs>() == get_startup_args(ver_tag);
  }
  return false;
}

bool is_sync_packet(PacketEnumeration id) {
  return id == PacketEnumeration::ID_AIM_SYNC ||
    id == PacketEnumeration::ID_VEHICLE_SYNC ||
    id == PacketEnumeration::ID_BULLET_SYNC ||
    id == PacketEnumeration::ID_SPECTATOR_SYNC ||
    id == PacketEnumeration::ID_PASSENGER_SYNC ||
    id == PacketEnumeration::ID_TRAILER_SYNC ||
    id == PacketEnumeration::ID_UNOCCUPIED_SYNC ||
    id == PacketEnumeration::ID_PLAYER_SYNC;
}

void start_subprocess() {
  const nlohmann::json startup_args = rak_state;

  auto daemon_path = std::filesystem::current_path() / R"(connd\connd-daemon-by-kin4stat.exe)";


  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  si.cb = sizeof(si);

  ipc_server server;
  if (!server.init("127.0.0.1", 0)) {
    LOG_ERROR("Server creation error");
    return;
  }

  LOG_INFO("Created server on {}:{}", "127.0.0.1", server.get_bound_port());

  auto run_cmd = std::format("{} {}", daemon_path.string(), server.get_bound_port());

  CreateProcessA(NULL,
                 run_cmd.data(),
                 nullptr,
                 nullptr,
                 FALSE,
                 DETACHED_PROCESS,
                 NULL,
                 NULL,
                 &si, &pi);

  server.accept();

  server.send_string("parameters", startup_args.dump());

  server.disconnect();

  LOG_INFO("Sent data to daemon");
}

void load_samp_state() {
  std::ifstream input{R"(.\connd\connd_samp_dump.json)"};

  input >> samp_state;
}

void read_settings() {
  std::ifstream input{R"(.\connd\connd-settings.json)"};

  nlohmann::json j;

  input >> j;

  try {
    g_settings = j.get<settings>();

    g_settings.default_timeout = std::clamp(g_settings.default_timeout, 0u, 10u);
    g_settings.quit_timeout = std::clamp(g_settings.quit_timeout, 0u, 10u);
  }
  catch (const std::exception&) {
    g_settings.default_timeout = 3;
    g_settings.quit_timeout = 3;

    input.close();

    std::ofstream output{R"(.\connd\connd-settings.json)"};

    output << nlohmann::json{g_settings}.dump();
  }

  LOG_INFO("Settings: default_timeout = {}m; quit_timeout = {}m", g_settings.default_timeout, g_settings.quit_timeout);
}

void get_subprocess_port() {
  std::ifstream input{R"(.\connd\connd_port.json)"};
  input >> g_settings.port;
}

bool receive_startup_data() {
  get_subprocess_port();

  client.init();

  LOG_INFO("Daemon opened on port {}, trying to connect", g_settings.port);

  if (!client.connect("127.0.0.1", g_settings.port)) {
    LOG_ERROR("Couldn't connect to deamon");

    return false;
  }

  auto [data, name] = client.recv_string();
  if (name == "parameters") {
    rak_state = nlohmann::json::parse(data).get<raknet_state>();

    LOG_INFO("Received startup parameters");
  }

  return true;
}

void alloc_console() {
#ifdef _DEBUG
  AllocConsole();

  FILE* fDummy;
  freopen_s(&fDummy, "CONIN$", "r", stdin);
  freopen_s(&fDummy, "CONOUT$", "w", stderr);
  freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif
}

UTILS_BEGIN_FUNCTION(safe_disconnect, void)
  LOG_INFO("Safe disconnect");

  utils::SetRaw(sampapi::GetAddress(Addresses<decltype(version)>::reset_nop), "\xC2\x04\x00", 3);
  utils::SetRaw(sampapi::GetAddress(Addresses<decltype(version)>::notification_nop), "\xC2\x10\x00", 3);
  utils::MemoryFill(sampapi::GetAddress(Addresses<decltype(version)>::clear_remote_system_nop), 0x90, 0x30);
  rak_state.pindex = RefNetGame(version)->GetRakClient()->GetPlayerIndex();

  RefNetGame(version)->GetRakClient()->Disconnect(0, 0);

  LOG_INFO("RakNet disconnected");
UTILS_END_FUNCTION(safe_disconnect, void)

void save_samp_state() {
  if (std::filesystem::exists(R"(.\connd\connd_samp_dump.json)")) {
    std::ifstream input{R"(.\connd\connd_samp_dump.json)"};

    nlohmann::json j;
    input >> j;

    samp_state["startup_args"] = j["startup_args"];

    if (!can_restore()) {
      return;
    }
  }

  safe_disconnect();

  auto rakpeer = samp_utils::get_rakpeer();
  auto rss = rakpeer->remote_system_list;

  rak_state.server_pid = rss->playerId;
  rak_state.local_port = rakpeer->myPlayerId.port;

  rak_state.received_packets_base_index = rss->reliabilityLayer.receivedPacketsBaseIndex;
  rak_state.send_number = rss->reliabilityLayer.message_number;

  std::ranges::copy(rss->reliabilityLayer.waitingForOrderedPacketWriteIndex,
                    std::begin(rak_state.waitingForOrderedPacketWriteIndex));
  std::ranges::copy(rss->reliabilityLayer.waitingForSequencedPacketWriteIndex,
                    std::begin(rak_state.waitingForSequencedPacketWriteIndex));
  std::ranges::copy(rss->reliabilityLayer.waitingForOrderedPacketReadIndex,
                    std::begin(rak_state.waitingForOrderedPacketReadIndex));
  std::ranges::copy(rss->reliabilityLayer.waitingForSequencedPacketReadIndex,
                    std::begin(rak_state.waitingForSequencedPacketReadIndex));

  LOG_INFO("Saved network state");

  samp_state.clear();

  auto store = [](auto version_tag) {
    samp_state.emplace("startup_args", get_startup_args(version_tag));

    LOG_INFO("Saved startup_args");

    camera_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved camera_state");
    remove_building_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved remove_building_state");
    remote_players_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved remote_players_state");
    netgame_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved netgame_state");
    labels_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved labels_state");
    local_player_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved local_player_state");
    gangzone_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved gangzone_state");
    chat_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved chat_state");
    dialog_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved dialog_state");
    actors_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved actors_state");
    net_stats_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved net_stats_state");
    kill_list_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved kill_list_state");
    textdraw_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved textdraw_state");
    vehicles_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved vehicles_state");
    objects_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved objects_state");
    pickups_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved pickups_state");
    checkpoint_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved checkpoint_state");
    object_edit_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved object_edit_state");
    object_selection_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved object_selection_state");
    markers_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved markers_state");
    menus_state::instance()->store(version_tag, samp_state);
    LOG_INFO("Saved menus_state");
  };

  if (utils::get_samp_version() == utils::samp_version::kR1) {
    auto ver_tag = sampapi::v037r1::VersionTag{};
    store(ver_tag);
  }
  else if (utils::get_samp_version() == utils::samp_version::kR3) {
    auto ver_tag = sampapi::v037r3::VersionTag{};
    store(ver_tag);
  }

  std::ofstream output{R"(.\connd\connd_samp_dump.json)"};
  output << samp_state.dump(2);

  output.close();
  LOG_INFO("Starting subprocess...");

  start_subprocess();

  LOG_INFO("Saved state");
}

void restore_samp_state() {
  auto restore = [](auto version_tag) {
    remove_building_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored remove_building_state");
    netgame_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored netgame_state");
    local_player_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored local_player_state");
    remote_players_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored remote_players_state");
    actors_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored actors_state");

    vehicles_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored vehicles_state");

    labels_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored labels_state");
    chat_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored chat_state");
    kill_list_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored kill_list_state");
    gangzone_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored gangzone_state");
    net_stats_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored net_stats_state");
    pickups_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored pickups_state");

    textdraw_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored textdraw_state");
    dialog_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored dialog_state");
    objects_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored objects_state");
    checkpoint_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored checkpoint_state");
    markers_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored markers_state");
    menus_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored menus_state");

    object_selection_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored object_selection_state");
    object_edit_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored object_edit_state");
    remote_players_state::instance()->restore_phase2(version_tag, samp_state);
    LOG_INFO("Restored remote_players_state(phase 2)");

    camera_state::instance()->restore(version_tag, samp_state);
    LOG_INFO("Restored camera_state");
    local_player_state::instance()->restore_phase2(version_tag, samp_state);
    LOG_INFO("Restored local_player_state(phase 2)");
  };

  if (utils::get_samp_version() == utils::samp_version::kR1) {
    auto ver_tag = sampapi::v037r1::VersionTag{};

    restore(ver_tag);
  }
  else if (utils::get_samp_version() == utils::samp_version::kR3) {
    auto ver_tag = sampapi::v037r3::VersionTag{};

    restore(ver_tag);
  }

  LOG_INFO("Gettings rpcs...");
  client.send_string("get_rpcs", "get_rpcs");

  auto i = 0u;
  while (true) {
    auto [stop_receiving, event_name] = client.recv_raw_data<int>();
    if (stop_receiving) break;

    auto [str, name] = client.recv_string();
    if (name == "raw_rpc") {
      auto en = static_cast<RPCEnumeration>(str[0]);
      if (str.size() == 1) {
        BitStream null_bs{};
        samp_utils::emul_rpc_bs(en, null_bs);
      }
      else {
        samp_utils::emul_rpc(en, std::span<char>{
                               str.data() + 1, str.size() - 2
                             });
      }

      ++i;
    }
  }

  LOG_INFO("Restored rpcs(total count {})", i);

  current_state = EmulState::kNone;

  auto rakpeer = samp_utils::get_rakpeer();
  auto rss = rakpeer->remote_system_list;

  rss->reliabilityLayer.receivedPacketsBaseIndex = rak_state.received_packets_base_index;
  rss->reliabilityLayer.message_number = rak_state.send_number;

  std::ranges::copy(rak_state.waitingForOrderedPacketWriteIndex,
                    std::begin(rss->reliabilityLayer.waitingForOrderedPacketWriteIndex));
  std::ranges::copy(rak_state.waitingForSequencedPacketWriteIndex,
                    std::begin(rss->reliabilityLayer.waitingForSequencedPacketWriteIndex));
  std::ranges::copy(rak_state.waitingForOrderedPacketReadIndex,
                    std::begin(rss->reliabilityLayer.waitingForOrderedPacketReadIndex));
  std::ranges::copy(rak_state.waitingForSequencedPacketReadIndex,
                    std::begin(rss->reliabilityLayer.waitingForSequencedPacketReadIndex));

  LOG_INFO("Restored reliability level");
}

void process_update() {
  if (current_state == EmulState::kCanEmulCookie) {
    samp_utils::emulate_socket_packet(rak_state.cookie_dump);
    current_state = EmulState::kWaitingForSecondRequest;
    LOG_INFO("Pass EmulState::kCanEmulCookie");
  }
  else if (current_state == EmulState::kCanEmulReply) {
    current_state = EmulState::kWaitingForThirdRequest;
    samp_utils::emulate_socket_packet(rak_state.reply_dump);
    LOG_INFO("Pass EmulState::kCanEmulReply");
  }
  else if (current_state == EmulState::kCanEmulAuthKey) {
    samp_utils::emulate_packet(rak_state.auth_key_dump);
    current_state = EmulState::kWaitingForAuthKey;
    LOG_INFO("Pass EmulState::kCanEmulAuthKey");
  }
  else if (current_state == EmulState::kCanEmulConnectionAccept) {
    auto rakpeer = samp_utils::get_rakpeer();
    auto rss = rakpeer->remote_system_list;

    BitStream in_bs(reinterpret_cast<unsigned char*>(rak_state.conn_accept_dump.data()),
                    rak_state.conn_accept_dump.size(), false);
    PlayerID externalID;

    in_bs.IgnoreBits(8);
    in_bs.Read(externalID.binaryAddress);
    in_bs.Read(externalID.port);

    rss->myExternalPlayerId = externalID;
    rss->connectMode = RemoteSystemStruct::CONNECTED;

    rss->reliabilityLayer.key_set = rss->setAESKey;

    samp_utils::emulate_packet(rak_state.conn_accept_dump);

    current_state = EmulState::kWaitingForClientJoin;

    LOG_INFO("Pass EmulState::kCanEmulConnectionAccept");
  }
  else if (current_state == EmulState::kCanEmulStaticData) {
    auto rakpeer = samp_utils::get_rakpeer();
    auto rss = rakpeer->remote_system_list;

    rss->staticData.Reset();
    rss->staticData.Write(rak_state.static_data_dump.data() + 1, rak_state.static_data_dump.size() - 1);

    samp_utils::emulate_packet(rak_state.static_data_dump);

    LOG_INFO("Pass EmulState::kCanEmulStaticData");
  }
  if (current_state == EmulState::kCanEmulStaticData) {
    LOG_INFO("Restoring samp state");
    restore_samp_state();
    LOG_INFO("State restored, good luck");
  }
}

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo) {
  __try {
    save_samp_state();
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
    LOG_INFO("Exception while saving");
    std::quick_exit(0);
  }

  std::quick_exit(0);
}

__declspec(naked) void htons_hook() {
  __asm {
    cmp set_port, 0
    je DONT_SET
    mov ax, port
    DONT_SET:
    mov word ptr [esp+0x12],ax
    jmp htons_jmp_back
    }
}

struct connd {
  using CNetGame__Dctor = void(CTHISCALL*)(void*);
  using cmd_handler = void(CCDECL*)(const char*);
  using RakClientFactory_Get = void*(CCDECL*)();
  using Receive = Packet*(CTHISCALL*)(void*);
  using Send = bool(CTHISCALL*)(void*, BitStream*, PacketPriority, PacketReliability, int, PlayerID, bool);
  using SendImmediate = bool(CTHISCALL*)(void*, unsigned char*, int, int, int, unsigned char, PlayerID, bool,
                                         bool, RakNetTimeNS);
  using SendTo = int(CTHISCALL*)(void*, SOCKET, const char*, int, unsigned, unsigned short);
  using CHud__DrawRadar = void(CCDECL*)();
  using RakClient__Connect = bool(CTHISCALL*)(void*, const char*, unsigned short, unsigned short, int, int);

  using CTimer__Update = void(CCDECL*)();
  using ProcessNetworkPacket = void(CSTDCALL*)(int, unsigned short, unsigned char*, std::size_t, RakPeer*);
  using RPCHandler = void(CCDECL*)(RPCParameters*);

  connd() {
    auto init = [this]<typename VerTag>(VerTag version_tag) {
      logger::instance();

      read_settings();

      rak_state.timeout = g_settings.default_timeout * 60;

      auto rpch = rpc_handler::instance();

      auto to_samp = [](std::uintptr_t address) {
        return sampapi::GetAddress(address);
      };

      utils::MemoryFill(to_samp(Addresses<VerTag>::received_packets_nop), 0x90, 9);

      rakclient_factory_hook.set_dest(to_samp(Addresses<VerTag>::rakclient_factory));
      send_hook.set_dest(to_samp(Addresses<VerTag>::send_hook));
      recv_hook.set_dest(to_samp(Addresses<VerTag>::recv_hook));
      connect_hook.set_dest(to_samp(Addresses<VerTag>::connect_hook));
      send_immediate_hook.set_dest(to_samp(Addresses<VerTag>::send_immediate_hook));
      process_packet_hook.set_dest(to_samp(Addresses<VerTag>::process_packet_hook));
      send_to_hook.set_dest(to_samp(Addresses<VerTag>::send_to_hook));
      quit_handler_hook.set_dest(to_samp(Addresses<VerTag>::quit_hook));
      netgame_dctor_hook.set_dest(to_samp(Addresses<VerTag>::netgame_dctor_hook));

      rpch->init(version_tag);
      dialog_state::instance()->init(version_tag, rpch);
      actors_state::instance()->init(rpch);
      remote_players_state::instance()->init(rpch);
      netgame_state::instance()->init(rpch);
      labels_state::instance()->init(rpch);
      local_player_state::instance()->init(rpch);
      gangzone_state::instance()->init(rpch);
      chat_state::instance()->init(rpch);
      kill_list_state::instance()->init(rpch);
      net_stats_state::instance()->init(rpch);
      textdraw_state::instance()->init(rpch);
      vehicles_state::instance()->init(rpch);
      objects_state::instance()->init(rpch);
      pickups_state::instance()->init(rpch);
      checkpoint_state::instance()->init(rpch);
      object_edit_state::instance()->init(rpch);
      object_selection_state::instance()->init(rpch);
      markers_state::instance()->init(rpch);
      remove_building_state::instance()->init(rpch);
      menus_state::instance()->init(rpch);
      camera_state::instance()->init(rpch);

      auto hook_address = sampapi::GetAddress(Addresses<VerTag>::port_change_hook);
      auto relative_address = kthook::detail::get_relative_address(reinterpret_cast<std::uintptr_t>(&htons_hook),
                                                                   hook_address);

      htons_jmp_back = hook_address + 5;
      utils::SetRaw(hook_address, "\xE9", 1);
      utils::SetRaw(hook_address + 1, reinterpret_cast<char*>(&relative_address), 4);
    };

    samp_init_hook.set_dest(0x58A330);
    update_hook.set_dest(0x561B10);

    if (utils::get_samp_version() == utils::samp_version::kR1) {
      auto ver_tag = sampapi::v037r1::VersionTag{};
      init(ver_tag);
    }
    else if (utils::get_samp_version() == utils::samp_version::kR3) {
      auto ver_tag = sampapi::v037r3::VersionTag{};
      init(ver_tag);
    }

    rakclient_factory_hook.set_cb([](const auto& hook) {
      void* rakclient = hook.get_trampoline()();

      g_rakclient = rakclient;
      g_rakpeer = reinterpret_cast<RakPeer*>(static_cast<char*>(rakclient) - 0xDDE);

      LOG_INFO("Got RakClient address: 0x{:08X}", reinterpret_cast<std::uintptr_t>(g_rakclient));

      return rakclient;
    });

    process_packet_hook.set_cb(
      [](const auto& hook, int addr, unsigned short port, unsigned char* data, std::size_t len, RakPeer* rakpeer) {
        auto packet_id_uchar = *data;
        auto packet_id = static_cast<PacketEnumeration>(packet_id_uchar);
        if (current_state == EmulState::kNone) {
          if (packet_id == PacketEnumeration::ID_OPEN_CONNECTION_COOKIE && rak_state.cookie_dump.empty()) {
            rak_state.cookie_dump.resize(len);

            std::memcpy(rak_state.cookie_dump.data(), data, len);
          }
          else if (packet_id == PacketEnumeration::ID_OPEN_CONNECTION_REPLY && rak_state.reply_dump.empty()) {
            rak_state.reply_dump.resize(len);

            std::memcpy(rak_state.reply_dump.data(), data, len);
          }
        }
        else {
          if (current_state == EmulState::kCanEmulCookie && packet_id ==
            PacketEnumeration::ID_OPEN_CONNECTION_COOKIE) {
            return hook.get_trampoline()(addr, port, data, len, rakpeer);
          }
          if (current_state == EmulState::kWaitingForThirdRequest && packet_id ==
            PacketEnumeration::ID_OPEN_CONNECTION_REPLY) {
            return hook.get_trampoline()(addr, port, data, len, rakpeer);
          }
          return;
        }

        return hook.get_trampoline()(addr, port, data, len, rakpeer);
      });

    send_hook.set_cb([this](const auto& hook,
                            void* rakpeer,
                            BitStream* bs,
                            PacketPriority priority,
                            PacketReliability reliability,
                            int ordering_channel,
                            PlayerID to,
                            bool broadcast) {
      auto packet_id_uchar = *bs->GetData();
      auto packet_id = static_cast<PacketEnumeration>(packet_id_uchar);
      auto rpc_id = static_cast<RPCEnumeration>(bs->GetData()[1]);

      if (current_state == EmulState::kWaitingForAuthKey &&
        packet_id == PacketEnumeration::ID_AUTH_KEY) {
        current_state = EmulState::kCanEmulConnectionAccept;
        return true;
      }

      if (current_state == EmulState::kWaitingForClientJoin &&
        packet_id == PacketEnumeration::ID_RPC &&
        rpc_id == RPCEnumeration::RPC_ClientJoin) {
        current_state = EmulState::kCanEmulStaticData;
        return true;
      }

      if (current_state == EmulState::kCanEmulStaticData &&
        packet_id == PacketEnumeration::ID_RPC &&
        (rpc_id == RPCEnumeration::RPC_RequestClass ||
        rpc_id == RPCEnumeration::RPC_Spawn)) {
        return true;
      }

      if (packet_id == ID_AUTH_KEY) {
        rak_state.server_pid = to;
      }

      return hook.get_trampoline()(rakpeer, bs, priority, reliability, ordering_channel, to, broadcast);
    });

    recv_hook.set_cb([this](const auto& hook, void* rakpeer) {
      auto packet = hook.get_trampoline()(rakpeer);
      if (packet && packet->data) {
        auto packet_id_uchar = *packet->data;
        auto packet_id = static_cast<PacketEnumeration>(packet_id_uchar);
        if (packet_id == PacketEnumeration::ID_AUTH_KEY) {
          rak_state.auth_key_dump.resize(packet->length);

          std::memcpy(rak_state.auth_key_dump.data(), packet->data, packet->length);
        }
        else if (packet_id == PacketEnumeration::ID_CONNECTION_REQUEST_ACCEPTED) {
          rak_state.conn_accept_dump.resize(packet->length);

          std::memcpy(rak_state.conn_accept_dump.data(), packet->data, packet->length);
        }
        else if (packet_id == PacketEnumeration::ID_RECEIVED_STATIC_DATA) {
          rak_state.static_data_dump.resize(packet->length);

          std::memcpy(rak_state.static_data_dump.data(), packet->data, packet->length);
        }
      }

      return packet;
    });

    send_to_hook.set_cb([](const auto& hook, void* sock, SOCKET s, const char* data, int len, unsigned addr,
                           unsigned short port) {
      if (data) {
        auto packet_id_uchar = *reinterpret_cast<const unsigned char*>(data);
        auto packet_id = static_cast<PacketEnumeration>(packet_id_uchar);

        if (current_state == EmulState::kWaitingForFirstRequest &&
          packet_id == PacketEnumeration::ID_OPEN_CONNECTION_REQUEST) {
          current_state = EmulState::kCanEmulCookie;
          return 1;
        }
        if (current_state == EmulState::kWaitingForSecondRequest &&
          packet_id == PacketEnumeration::ID_OPEN_CONNECTION_REQUEST) {
          current_state = EmulState::kCanEmulReply;
          return 1;
        }
      }

      return hook.get_trampoline()(sock, s, data, len, addr, port);
    });

    send_immediate_hook.set_cb([](const auto& hook,
                                  void* rakpeer,
                                  unsigned char* data,
                                  int number_of_bits,
                                  int priority,
                                  int reliability,
                                  unsigned char ordering_channel,
                                  PlayerID to,
                                  bool broadcast,
                                  bool useCallerDataAllocation,
                                  RakNetTimeNS currentTime) {
      auto packet_id_uchar = *data;
      auto packet_id = static_cast<PacketEnumeration>(packet_id_uchar);

      if (current_state == EmulState::kWaitingForThirdRequest &&
        packet_id == PacketEnumeration::ID_CONNECTION_REQUEST) {
        current_state = EmulState::kCanEmulAuthKey;
        return true;
      }

      return hook.get_trampoline()(rakpeer, data, number_of_bits, priority, reliability, ordering_channel, to,
                                   broadcast, useCallerDataAllocation, currentTime);
    });

    samp_init_hook.set_cb([this](const auto& hook) {
      hook.get_trampoline()();

      if (init_once) {
        if (std::filesystem::exists(R"(.\connd\connd_samp_dump.json)")) {
          load_samp_state();
          if (can_restore()) {
            if (receive_startup_data()) {
              current_state = EmulState::kWaitingForFirstRequest;
            }
            std::filesystem::remove(R"(.\connd\connd_samp_dump.json)");
          }
        }
      }

      init_once = false;
    });

    connect_hook.set_cb([this](const auto& hook,
                               void* rakpeer,
                               const char* hostname,
                               unsigned short server_port,
                               unsigned short hostport,
                               int unused,
                               int thread_sleep_timer) {
      if (set_filter_once) {
        SetUnhandledExceptionFilter(&ExceptionFilter);
        set_filter_once = false;

        LOG_INFO("SetUnhandledExceptionFilter");
      }
      if (current_state.load() == EmulState::kNone) {
        rak_state.hostname = hostname;
        rak_state.server_pid.port = server_port;
        rak_state.local_port = hostport;
      }
      else {
        set_port = true;
        port = htons(rak_state.local_port);

        LOG_INFO("Restoring connection to {}:{} on port {}(BE: {})", rak_state.hostname, rak_state.server_pid.port,
                 rak_state.local_port, port);

        return hook.get_trampoline()(rakpeer,
                                     rak_state.hostname.c_str(),
                                     rak_state.server_pid.port,
                                     rak_state.local_port,
                                     unused,
                                     thread_sleep_timer);
      }
      return hook.get_trampoline()(rakpeer,
                                   hostname,
                                   server_port,
                                   hostport,
                                   hostport,
                                   thread_sleep_timer);
    });

    update_hook.set_cb([](const auto& hook) {
      if (GetKeyState(VK_F5) & 0x8000 && GetKeyState(VK_CONTROL) & 0x8000) {
        LOG_INFO("Emulating crash");

        save_samp_state();
        std::quick_exit(0);
      }

      __try {
        process_update();
      }
      __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_INFO("Exception while restoring state, falling back");

        current_state = EmulState::kNone;

        auto reset_state = [](auto version_tag) {
          RefNetGame(version_tag)->GetRakClient()->Disconnect(0, 0);
          RefNetGame(version_tag)->ShutdownForRestart();
        };

        if (utils::get_samp_version() == utils::samp_version::kR1) {
          auto ver_tag = sampapi::v037r1::VersionTag{};
          RefNetGame(ver_tag)->m_nGameState = 9;
        }
        else if (utils::get_samp_version() == utils::samp_version::kR3) {
          auto ver_tag = sampapi::v037r3::VersionTag{};
          RefNetGame(ver_tag)->m_nGameState = 1;
        }
      }
      hook.get_trampoline()();
    });

    quit_handler_hook.set_cb([](const auto& hook, const char* p) {
      std::string_view params{p};

      if (!params.empty()) {
        int result;

        auto [ptr, ec]{std::from_chars(params.data(), params.data() + params.size(), result)};

        if (ec == std::errc()) {
          result = std::clamp(result, 0, 10);

          rak_state.timeout = result * 60;
        }
        else {
          rak_state.timeout = g_settings.quit_timeout * 60;
        }
      }
      else {
        rak_state.timeout = g_settings.quit_timeout * 60;
      }

      safe_disconnect();

      return hook.get_trampoline()(p);
    });

    netgame_dctor_hook.set_cb([](const auto& hook, void* netgame) {
      if (g_settings.quit_timeout) {
        LOG_INFO("Quiting from /q; daemon timeout: {}", rak_state.timeout);

        save_samp_state();
      }
      return hook.get_trampoline()(netgame);
    });

    send_hook.install();
    recv_hook.install();
    samp_init_hook.install();
    send_to_hook.install();

    send_immediate_hook.install();

    connect_hook.install();
    update_hook.install();
    process_packet_hook.install();
    rakclient_factory_hook.install();
    quit_handler_hook.install();
    netgame_dctor_hook.install();
  }

  kthook::kthook_simple<cmd_handler> quit_handler_hook;
  kthook::kthook_simple<CNetGame__Dctor> netgame_dctor_hook;
  kthook::kthook_simple<SendTo> send_to_hook;
  kthook::kthook_simple<ProcessNetworkPacket> process_packet_hook;
  kthook::kthook_simple<CTimer__Update> update_hook;
  kthook::kthook_simple<RakClient__Connect> connect_hook;
  kthook::kthook_simple<Send> send_hook;
  kthook::kthook_simple<SendImmediate> send_immediate_hook;
  kthook::kthook_simple<Receive> recv_hook;
  kthook::kthook_simple<CHud__DrawRadar> samp_init_hook;
  kthook::kthook_simple<RakClientFactory_Get> rakclient_factory_hook;
} p{};

BOOL WINAPI DllMain(HMODULE hinstDLL,
                    DWORD fdwReason,
                    LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    char temp[1024];

    GetModuleFileNameA(hinstDLL, temp, sizeof(temp));

    if (std::string_view{temp}.find("connd-sa-by-kin4stat") == std::string_view::npos) {
      LOG_INFO("Plugin name changed, aborting");
      return FALSE;
    }
    if (GetModuleHandleA("connd-sa-by-kin4stat.asi") != hinstDLL &&
      GetModuleHandleA("connd-sa-by-kin4stat.dll") != hinstDLL) {

      LOG_INFO("Plugin name changed, aborting");
      return FALSE;
    }

    alloc_console();
  }

  return TRUE;
}
