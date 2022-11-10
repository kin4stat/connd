#pragma once
#include <string>
#include <string_view>

#ifdef __RESHARPER__
#define WRAP_VERSION sampapi::v037r1::VersionTag
#else
#define WRAP_VERSION auto
#endif

struct PlayerID;
class RakPeer;
struct RPCParameters;
class CPhysical;

namespace utils {
  enum class samp_version {
    kR1,
    kR3,
    kUnknown,
  };

  bool is_samp_loaded();
  samp_version get_samp_version();

  void MemoryFill(std::uintptr_t address, std::uint8_t value, std::size_t size, bool protect = true);
  void SetRaw(std::uintptr_t address, const char* RawData, std::size_t size, bool protect = true);
}
