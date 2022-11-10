#pragma once
#include <optional>
#include <string>
#include <string_view>

inline std::wstring utf8_to_utf16(std::string_view utf8) {
  if (utf8.empty()) return L"";
  std::wstring utf16;
  const int utf8_len = static_cast<int>(utf8.length());
  auto utf8_data = reinterpret_cast<const char*>(utf8.data());

  const int utf16_len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_data, utf8_len, nullptr, 0);
  if (utf16_len == 0) return utf16;
  utf16.resize(utf16_len);
  ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_data, utf8_len, utf16.data(), utf16.size());
  return utf16;
}

inline std::string utf16_to_utf8(std::wstring_view utf16) {
  if (utf16.empty()) return "";
  std::string utf8;
  const int utf16_len = static_cast<int>(utf16.length());
  const wchar_t* utf16_data = utf16.data();

  const int utf8_len = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16_data, utf16_len, nullptr, 0, 0, 0);
  if (utf8_len == 0) return utf8;
  utf8.resize(utf8_len);
  auto utf8_data = reinterpret_cast<char*>(utf8.data());
  ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16_data, utf16_len, utf8_data, utf8.size(), 0, 0);
  return utf8;
}

inline std::wstring cpacp_to_utf16(std::string_view cpacp) {
  if (cpacp.empty()) return L"";
  std::wstring utf16;
  const char* cpacp_data = cpacp.data();
  const int cpacp_len = static_cast<int>(cpacp.length());

  const int utf16_len = ::MultiByteToWideChar(CP_ACP, 0, cpacp_data, cpacp_len, nullptr, 0);
  if (utf16_len == 0) return L"";
  utf16.resize(utf16_len);
  ::MultiByteToWideChar(CP_ACP, 0, cpacp_data, cpacp_len, utf16.data(), utf16.size());
  return utf16;
}

inline std::string cpacp_to_utf8(std::string_view cpacp) {
  if (cpacp.empty()) return "";
  std::wstring utf16;
  const char* cpacp_data = cpacp.data();
  const int cpacp_len = static_cast<int>(cpacp.length());

  const int utf16_len = ::MultiByteToWideChar(CP_ACP, 0, cpacp_data, cpacp_len, nullptr, 0);
  if (utf16_len == 0) return "";
  utf16.resize(utf16_len);
  ::MultiByteToWideChar(CP_ACP, 0, cpacp_data, cpacp_len, utf16.data(), utf16.size());
  return utf16_to_utf8(utf16);
}

inline std::string utf8_to_cpacp(std::string_view utf8) {
  if (utf8.empty()) return "";
  std::string cpacp;
  std::wstring utf16 = utf8_to_utf16(utf8);
  const int utf16_len = static_cast<int>(utf16.length());
  const wchar_t* utf16_data = utf16.c_str();
  const int cpacp_len = ::WideCharToMultiByte(CP_ACP, 0, utf16_data, utf16_len, nullptr, 0, 0, 0);
  if (cpacp_len == 0) return "";
  cpacp.resize(cpacp_len);
  ::WideCharToMultiByte(CP_ACP, 0, utf16_data, utf16_len, cpacp.data(), cpacp.size(), 0, 0);
  return cpacp;
}

inline std::string cp2utf(std::string_view cpacp) {
  return cpacp_to_utf8(cpacp);
}

inline std::string utf2cp(std::string_view utf8) {
  return utf8_to_cpacp(utf8);
}