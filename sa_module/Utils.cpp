#include "Utils.hpp"

#include <d3d9.h>
#include "Windows.h"
#include "RakNet.hpp"
#include "raknet_state.hpp"
#include "sampapi/CGame.h"
#include "sampapi/0.3.7-R3-1/CNetGame.h"

bool utils::is_samp_loaded() {
  return GetModuleHandleA("samp.dll") != 0;
}

utils::samp_version utils::get_samp_version() {
  static samp_version ver = []() {
    auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("samp.dll"));
    auto ntheader = reinterpret_cast<IMAGE_NT_HEADERS*>(base + reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
    // NOLINT(performance-no-int-to-ptr)
    auto ep = ntheader->OptionalHeader.AddressOfEntryPoint;
    switch (ep) {
    case 0x31DF13:
      return samp_version::kR1;
    case 0xCC4D0:
      return samp_version::kR3;
    default:
      return samp_version::kUnknown;
    //case 0xCBCB0:  return samp_version::SAMP_0_3_7_R4;
    }
  }();
  return ver;
}


void utils::MemoryFill(std::uintptr_t address, std::uint8_t value, std::size_t size, bool protect) {
  DWORD oldProt;
  if (protect) VirtualProtect(reinterpret_cast<LPVOID>(address), 8, PAGE_READWRITE, &oldProt);
  memset(reinterpret_cast<void*>(address), value, size);
  if (protect) VirtualProtect(reinterpret_cast<LPVOID>(address), 8, oldProt, &oldProt);
}

void utils::SetRaw(std::uintptr_t address, const char* RawData, std::size_t size, bool protect) {
  DWORD oldProt;
  if (protect) VirtualProtect(reinterpret_cast<LPVOID>(address), 8, PAGE_READWRITE, &oldProt);
  memcpy(reinterpret_cast<void*>(address), RawData, size);
  if (protect) VirtualProtect(reinterpret_cast<LPVOID>(address), 8, oldProt, &oldProt);
}
