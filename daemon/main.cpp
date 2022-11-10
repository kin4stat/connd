#include <cstdlib>
#include <ctime>
#include <thread>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "raknet_plugin.hpp"
#include "rakclient.hpp"
#include "utils.hpp"

#include "raknet_state.hpp"
#include "ipc/ipc_client.hpp"
#include "ipc/ipc_server.hpp"

#include "RakNet/RakNetworkFactory.h"

#include "logger.hpp"

RakPlugin* plugin;
std::atomic<bool> stop = false;

void recv_startup_parameters(unsigned short port) {
  LOG_INFO("Receiving data from {}:{}", "127.0.0.1", port);

  ipc_client client;

  client.init();
  client.connect("127.0.0.1", port);

  auto [data, name] = client.recv_string();
  if (name == "parameters") {
    rak_state = nlohmann::json::parse(data).get<raknet_state>();

    LOG_INFO("Startup data received");
  }
}

void daemon() {
  ipc_server server;
  server.init("127.0.0.1", 0);

  std::ofstream output{ R"(.\connd\connd_port.json)" };
  output << server.get_bound_port() << std::endl;
  output.close();

  LOG_INFO("Started daemon server on {}:{}", "127.0.0.1", server.get_bound_port());

  server.accept();

  stop = true;

  LOG_INFO("Stopping mainloop");

  g_rak_client->Disconnect(0, 0);

  LOG_INFO("RakNet Disconnected");

  auto& rel_level = get_pure_rak_client()->remoteSystemList->reliabilityLayer;

  std::ranges::copy(rel_level.waitingForOrderedPacketWriteIndex,
                    std::begin(rak_state.waitingForOrderedPacketWriteIndex));
  std::ranges::copy(rel_level.waitingForSequencedPacketWriteIndex,
                    std::begin(rak_state.waitingForSequencedPacketWriteIndex));
  std::ranges::copy(rel_level.waitingForOrderedPacketReadIndex,
                    std::begin(rak_state.waitingForOrderedPacketReadIndex));
  std::ranges::copy(rel_level.waitingForSequencedPacketReadIndex,
                    std::begin(rak_state.waitingForSequencedPacketReadIndex));

  rak_state.received_packets_base_index = rel_level.receivedPacketsBaseIndex;
  rak_state.send_number = rel_level.messageNumber;

  LOG_INFO("Saved reliability level");

  const nlohmann::json startup_args = rak_state;

  server.send_string("parameters", startup_args.dump());

  LOG_INFO("Sent network state");

  plugin->send_rpcs(&server);

  LOG_INFO("Sent rpcs, count: {}", plugin->get_rpc_count());

  server.disconnect();
}

int main(int argc, char* argv[]) {
  logger::instance();

  char temp[1024];

  GetModuleFileNameA(GetModuleHandleA(nullptr), temp, sizeof(temp));

  if (std::string_view{ temp }.find("connd-daemon-by-kin4stat.exe") == std::string_view::npos) {
    LOG_INFO("Daemon executable name changed, shutting down");

    return 1;
  }

  auto start = std::chrono::steady_clock::now();

  alloc_console();

  const auto port = std::stoul(argv[1]);

  recv_startup_parameters(port);

  plugin = new RakPlugin{};

  srand(time(nullptr));

  g_rak_client = RakNetworkFactory::GetRakClientInterface();

  g_rak_client->SetMTUSize(576);
  g_rak_client->SetPassword("");

  g_rak_client->AttachPlugin(plugin);
  while (!g_rak_client->Connect(rak_state.hostname.c_str(), rak_state.server_pid.port, rak_state.local_port, 0, 2)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto d_thread = std::thread{daemon};

  while (!stop) {
    while (auto pkt = g_rak_client->Receive()) {
      g_rak_client->DeallocatePacket(pkt);
    }

    std::chrono::duration<double> diff = std::chrono::steady_clock::now() - start;
    if (diff.count() > rak_state.timeout) {
      [[maybe_unused]] std::error_code ec{};

      std::filesystem::remove(R"(.\connd\connd_samp_dump.json)", ec);
      std::quick_exit(0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  LOG_INFO("Mainloop stopped, waiting for daemon thread");

  d_thread.join();

  LOG_INFO("Shutting down");

  return 0;
}
