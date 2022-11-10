#pragma once
#include <nlohmann/json.hpp>
#include <CQuaternion.h>

struct Quaternion {
  float x, y, z, w;

  Quaternion(float x, float y, float z, float w)
    : x(x),
      y(y),
      z(z),
      w(w) {
  }

  Quaternion()
    : x(0),
      y(0),
      z(0),
      w(0) {
  }

  Quaternion(CQuaternion quat)
    : x(quat.imag.x),
      y(quat.imag.y),
      z(quat.imag.z),
      w(quat.real) {
  }

  operator CQuaternion() {
    return CQuaternion{{x, y, z}, w};
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Quaternion, x, y, z, w);
