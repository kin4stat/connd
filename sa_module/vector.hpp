#pragma once
#include "sampapi/CVector.h"
#include "CVector.h"
#include <nlohmann/json.hpp>

struct Vector {
  float x, y, z;

  Vector(float x, float y, float z)
    : x(x),
      y(y),
      z(z) {
  }

  Vector()
    : x(0),
      y(0),
      z(0) {
  }

  Vector(::CVector vec)
    : x(vec.x),
      y(vec.y),
      z(vec.z) {
  }

  operator ::CVector() {
    return ::CVector{x, y, z};
  }

  Vector(sampapi::CVector vec)
    : x(vec.x),
      y(vec.y),
      z(vec.z) {
  }

  operator sampapi::CVector() {
    return sampapi::CVector{x, y, z};
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector, x, y, z);
