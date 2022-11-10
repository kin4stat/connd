#pragma once

#include "BitStream.hpp"
#include "crtp_singleton.hpp"
#include "kthook/kthook.hpp"

struct RPCParameters;

class rpc_handler final : public Singleton<rpc_handler> {
public:
  rpc_handler() = default;
  ~rpc_handler() override = default;

  template <typename VerTag>
  void init(WRAP_VERTAG version) {

    rpc_id_hook.set_dest(sampapi::GetAddress(Addresses<VerTag>::get_rpc_id_hook));
    rpc_with_params_hook.set_dest(sampapi::GetAddress(Addresses<VerTag>::rpc_with_params_hook));
    rpc_without_params_hook.set_dest(sampapi::GetAddress(Addresses<VerTag>::rpc_wo_params_hook));

    rpc_with_params_hook.set_cb([this](const kthook::kthook_naked& hook) {
      auto params = reinterpret_cast<RPCParameters*>(hook.get_context().eax);
      BitStream bs{params->input, params->numberOfBitsOfData / 8 + 1, true};

      on_incoming_rpc.emit(rpc_id, &bs);
    });

    rpc_without_params_hook.set_cb([this](const kthook::kthook_naked& hook) {
      auto params = reinterpret_cast<RPCParameters*>(hook.get_context().ecx);
      BitStream bs{};

      on_incoming_rpc.emit(rpc_id, &bs);
    });

    rpc_id_hook.set_cb([this](const kthook::kthook_naked& hook) {
      rpc_id = hook.get_context().ebx & 0xFF;
    });

    rpc_with_params_hook.install();
    rpc_without_params_hook.install();
    rpc_id_hook.install();
  }

  ktsignal::ktsignal<void(unsigned char idx, BitStream*)> on_incoming_rpc;

private:
  unsigned rpc_id{};

  kthook::kthook_naked rpc_with_params_hook;
  kthook::kthook_naked rpc_without_params_hook;
  kthook::kthook_naked rpc_id_hook;
};
