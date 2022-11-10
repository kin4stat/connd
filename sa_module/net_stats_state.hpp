#pragma once

#include <nlohmann/json.hpp>

#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "sampapi/CNetStats.h"

class rpc_handler;
class BitStream;

struct NetStats {
  unsigned long total_sent;
  unsigned long total_recv;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetStats, total_sent, total_recv)

class net_stats_state : public Singleton<net_stats_state> {
public:
  net_stats_state() = default;
  ~net_stats_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template<typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto net_stats = RefNetStats(version);

    to.emplace("net_stats", NetStats{
                 net_stats->m_dwLastTotalBytesSent,
                 net_stats->m_dwLastTotalBytesRecv,
               });
  }

  template<typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto this_state = from["net_stats"].get<NetStats>();

    auto net_stats = RefNetStats(version);
    net_stats->m_dwLastTotalBytesSent = this_state.total_sent;
    net_stats->m_dwLastTotalBytesRecv = this_state.total_recv;
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
