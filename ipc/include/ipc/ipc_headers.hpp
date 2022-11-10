#pragma once

#include <string>

struct header {
  std::size_t size;
};

struct string_recv {
  std::string string;
  std::string name;
};

template <typename T>
struct raw_data_recv {
  T data;
  std::string name;
};
