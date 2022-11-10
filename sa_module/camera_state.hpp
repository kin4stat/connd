#pragma once

#include <array>

#include <CCamera.h>
#include <CTimer.h>
#include "samp_types.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "cp2utf.hpp"
#include "sampapi/CGame.h"
#include "sampapi/CNetGame.h"

class rpc_handler;
class BitStream;


class camera_state : public Singleton<camera_state> {
public:
  camera_state() = default;
  ~camera_state() override = default;

  void init(rpc_handler* rpch) {
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_cam = RefGame(version)->GetCamera();
    auto object_pool = RefNetGame(version)->GetObjectPool();

    Camera this_state{};

    this_state.attached_to_object = 0xFFFF;

    if (samp_cam->m_pAttachedTo) {
      for (auto i = 0u; i < 1000; ++i) {
        if (object_pool->m_pObject[i] == samp_cam->m_pAttachedTo) {
          this_state.attached_to_object = i;
          break;
        }
      }
    }

    if (this_state.attached_to_object == 0xFFFF) {
      if (TheCamera.m_bLookingAtVector) {
        this_state.look_at_coords.emplace(TheCamera.m_vecFixedModeVector,
                                          TheCamera.m_vecFixedModeSource,
                                          TheCamera.m_vecFixedModeUpOffSet);
      }

      if (TheCamera.m_fTrackLinearEndTime - CTimer::m_snTimeInMilliseconds > 0) {
        this_state.move_cam = Camera::MoveCam{};
        this_state.move_cam->ticks_left = TheCamera.m_fTrackLinearEndTime - CTimer::m_snTimeInMilliseconds;
        this_state.move_cam->track = true;
        this_state.move_cam->from = TheCamera.m_vecTrackLinear;
        this_state.move_cam->to = TheCamera.m_vecTrackLinearEndPoint;
        this_state.move_cam->with_ease = TheCamera.m_bTrackLinearWithEase;
      }
      else if (TheCamera.m_fMoveLinearEndTime - CTimer::m_snTimeInMilliseconds > 0) {
        this_state.move_cam = Camera::MoveCam{};
        this_state.move_cam->ticks_left = TheCamera.m_fMoveLinearEndTime - CTimer::m_snTimeInMilliseconds;
        this_state.move_cam->track = true;
        this_state.move_cam->from = TheCamera.m_vecMoveLinear;
        this_state.move_cam->to = TheCamera.m_vecMoveLinearPosnEnd;
        this_state.move_cam->with_ease = TheCamera.m_bMoveLinearWithEase;
      }
    }

    to.emplace("camera", this_state);
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto samp_cam = RefGame(version)->GetCamera();
    auto object_pool = RefNetGame(version)->GetObjectPool();

    auto this_state = from["camera"].get<Camera>();

    if (this_state.attached_to_object != 0xFFFF) {
      samp_cam->Attach(object_pool->m_pObject[this_state.attached_to_object]);
      return;
    }
    if (this_state.look_at_coords.has_value()) {
      CVector pos = this_state.look_at_coords->position;
      CVector rotation = this_state.look_at_coords->rotation;
      CVector look_at = this_state.look_at_coords->look_at;
      TheCamera.TakeControlNoEntity(&look_at, 2, 1);
      TheCamera.SetCamPositionForFixedMode(&pos, &rotation);
    }
    else if (this_state.move_cam.has_value()) {
      auto func = &CCamera::VectorMoveLinear;
      if (this_state.move_cam->track)
        func = &CCamera::VectorTrackLinear;

      CVector start_point = this_state.move_cam->from;
      CVector end_point = this_state.move_cam->to;
      float ticks_left = this_state.move_cam->ticks_left;
      bool with_ease = this_state.move_cam->with_ease;
      std::invoke(func, &TheCamera, &start_point, &end_point, ticks_left, with_ease);
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream*, std::uint16_t pid) {
  }
};
