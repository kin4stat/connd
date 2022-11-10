#pragma once

#include "Utils.hpp"
#include "BitStream.hpp"
#include "RakNet.hpp"
#include "raknet_state.hpp"
#include "sampapi/sampapi.h"
#include "samp_addresses.hpp"

#define UTILS_BEGIN_FUNCTION(name, return_type, ...) inline auto impl_##name = [](auto version, __VA_ARGS__) -> return_type {
#define UTILS_END_FUNCTION(name, return_type) }; \
    inline auto name = [](auto&&... args) -> return_type { \
        auto ver = utils::get_samp_version();\
        if (ver == utils::samp_version::kR1) { \
            return impl_##name(sampapi::versions::v037r1, std::forward<decltype(args)>(args)...);\
        } \
        else if (ver ==  utils::samp_version::kR3) { \
            return impl_##name(sampapi::versions::v037r3, std::forward<decltype(args)>(args)...);\
        }\
        __assume(false);\
    };

void* g_rakclient;
RakPeer* g_rakpeer;

namespace samp_utils {
  using RPCHandler = void(__cdecl*)(RPCParameters*);

  UTILS_BEGIN_FUNCTION(get_rakpeer, RakPeer*)
    return g_rakpeer;
  UTILS_END_FUNCTION(get_rakpeer, RakPeer*)

  UTILS_BEGIN_FUNCTION(get_rpc_handler, RPCHandler, unsigned char idx)
    return impl_get_rakpeer(version)->map[idx]->staticFunctionPointer;
  UTILS_END_FUNCTION(get_rpc_handler, RPCHandler)

  UTILS_BEGIN_FUNCTION(emul_rpc_bs, void, RPCEnumeration idx, BitStream& bs)
    BitStream unused;

    RPCParameters params{};
    params.input = bs.GetData();
    params.numberOfBitsOfData = bs.GetNumberOfBitsUsed();
    params.recipient = impl_get_rakpeer(version);
    params.sender = rak_state.server_pid;
    params.replyToSender = &unused;

    auto handler = impl_get_rpc_handler(version, static_cast<unsigned char>(idx));
    handler(&params);
  UTILS_END_FUNCTION(emul_rpc_bs, void)

  UTILS_BEGIN_FUNCTION(emul_rpc, void, RPCEnumeration idx, std::span<char> data)
    auto inp = reinterpret_cast<const unsigned char*>(data.data());

    BitStream unused;

    RPCParameters params;
    params.input = const_cast<unsigned char*>(inp);
    params.numberOfBitsOfData = data.size() * 8;
    params.recipient = impl_get_rakpeer(version);
    params.sender = rak_state.server_pid;
    params.replyToSender = &unused;

    auto handler = impl_get_rpc_handler(version, static_cast<unsigned char>(idx));

    handler(&params);
  UTILS_END_FUNCTION(emul_rpc, void)

  UTILS_BEGIN_FUNCTION(snew, void*, std::size_t size)
    using VerTag = decltype(version);

    using this_sig_t = void*(__cdecl*)(std::size_t);

    auto function = reinterpret_cast<this_sig_t>(sampapi::GetAddress(Addresses<VerTag>::operator_new));

    return function(size);
  UTILS_END_FUNCTION(snew, void*)

  UTILS_BEGIN_FUNCTION(alloc_packet, Packet*, std::size_t size, void* data)
    using VerTag = decltype(version);

    using this_sig_t = Packet*(__cdecl*)(std::size_t, void*);

    auto function = reinterpret_cast<this_sig_t>(sampapi::GetAddress(Addresses<VerTag>::alloc_packet));
    return function(size, data);
  UTILS_END_FUNCTION(alloc_packet, Packet*)

  UTILS_BEGIN_FUNCTION(add_packet_to_producer, void, Packet* p)
    using VerTag = decltype(version);

    using this_sig_t = void(__thiscall*)(void*, Packet*);

    auto function = reinterpret_cast<this_sig_t>(sampapi::GetAddress(Addresses<VerTag>::add_packet_to_producer));

    return function(impl_get_rakpeer(version), p);
  UTILS_END_FUNCTION(add_packet_to_producer, void)

  UTILS_BEGIN_FUNCTION(emulate_packet, void, std::span<char> data)
    using VerTag = decltype(version);

    const auto packet_size = data.size();
    const auto packet_data = static_cast<char*>(impl_snew(version, packet_size));
    std::memcpy(packet_data, data.data(), packet_size);
    Packet* packet = impl_alloc_packet(version, packet_size, packet_data);

    packet->bitSize = packet_size * 8;
    packet->playerId = rak_state.server_pid;
    packet->playerIndex = rak_state.pindex;

    impl_add_packet_to_producer(version, packet);
  UTILS_END_FUNCTION(emulate_packet, void)

  UTILS_BEGIN_FUNCTION(emulate_socket_packet, void, std::span<char> data)
    using VerTag = decltype(version);

    using ProcessNetworkPacket = void(__stdcall*)(int, unsigned short, const char*, std::size_t, RakPeer*);

    auto function = reinterpret_cast<ProcessNetworkPacket>(sampapi::GetAddress(
      Addresses<VerTag>::process_network_packet));

    function(rak_state.server_pid.binaryAddress,
             rak_state.server_pid.port,
             data.data(),
             data.size(),
             impl_get_rakpeer(version));
  UTILS_END_FUNCTION(emulate_socket_packet, void)

  inline bool is_sync_packet(PacketEnumeration id) {
    return id == PacketEnumeration::ID_AIM_SYNC ||
      id == PacketEnumeration::ID_VEHICLE_SYNC ||
      id == PacketEnumeration::ID_BULLET_SYNC ||
      id == PacketEnumeration::ID_SPECTATOR_SYNC ||
      id == PacketEnumeration::ID_PASSENGER_SYNC ||
      id == PacketEnumeration::ID_TRAILER_SYNC ||
      id == PacketEnumeration::ID_UNOCCUPIED_SYNC ||
      id == PacketEnumeration::ID_PLAYER_SYNC;
  }
}
