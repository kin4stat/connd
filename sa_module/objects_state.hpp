#pragma once
#include <nlohmann/json.hpp>
#include "json_utils.hpp"

#include <optional>
#include <variant>

#include "samp_types.hpp"
#include "cp2utf.hpp"
#include "rpc_handler.hpp"
#include "Utils.hpp"
#include "crtp_singleton.hpp"
#include "RakNet.hpp"
#include "vector.hpp"
#include "RakNet/StringCompressor.h"
#include "sampapi/CNetGame.h"

class rpc_handler;
class BitStream;

class objects_state : public Singleton<objects_state> {
public:
  objects_state() {
    StringCompressor::AddReference();
  }

  ~objects_state() override {
    StringCompressor::RemoveReference();
  }

  void init(rpc_handler* rpch) {
    rpch->on_incoming_rpc += [this](unsigned char rpc_id, BitStream* bs) {
      auto rpc = static_cast<RPCEnumeration>(rpc_id);
      if (rpc == RPCEnumeration::RPC_ScrCreateObject ||
        rpc == RPCEnumeration::RPC_ScrSetObjectMaterial ||
        rpc == RPCEnumeration::RPC_ScrDestroyObject) {
        on_rpc(rpc, bs);
      }
    };
  }

  template <typename VerTag>
  void store(WRAP_VERTAG version, nlohmann::json& to) {
    auto samp_pool = RefNetGame(version)->GetObjectPool();

    auto& object_pool = to["object_pool"];
    for (auto i = 0u; i < 1000; ++i) {
      if (!samp_pool->m_bNotEmpty[i]) continue;
      auto object_info = samp_pool->m_pObject[i];
      if (!object_info) continue;

      Object this_state{};

      this_state._this_id = i;
      this_state.model_id = object_info->GetModelIndex();

      this_state.draw_distance = object_info->m_fDrawDistance;
      this_state.position = object_info->m_pGameEntity->GetPosition();
      this_state.rotation = object_info->m_rotation;
      if (object_info->m_nAttachedToObject != 0xFFFF ||
        object_info->m_nAttachedToVehicle != 0xFFFF) {
        this_state.attach_info.emplace(object_info->m_nAttachedToObject,
                                       object_info->m_nAttachedToVehicle,
                                       object_info->m_attachOffset,
                                       object_info->m_attachRotation,
                                       object_info->m_bSyncRotation);
      }
      this_state.dont_collide_with_camera = object_info->m_bDontCollideWithCamera;
      this_state.materials = std::move(material_info[i]);

      object_pool.emplace_back(this_state);
    }
  }

  template <typename VerTag>
  void restore(WRAP_VERTAG version, const nlohmann::json& from) {
    auto object_pool = RefNetGame(version)->GetObjectPool();

    for (auto& arr : from["object_pool"]) {
      auto this_state = arr.get<Object>();

      BitStream object_create{};
      object_create.Write<std::uint16_t>(this_state._this_id);
      object_create.Write<std::uint32_t>(this_state.model_id);
      object_create.Write<float>(this_state.position.x);
      object_create.Write<float>(this_state.position.y);
      object_create.Write<float>(this_state.position.z);
      object_create.Write<float>(this_state.rotation.x);
      object_create.Write<float>(this_state.rotation.y);
      object_create.Write<float>(this_state.rotation.z);
      object_create.Write<float>(this_state.draw_distance);
      object_create.Write<std::uint8_t>(this_state.dont_collide_with_camera);
      if (this_state.attach_info.has_value()) {
        object_create.Write<std::uint16_t>(this_state.attach_info->attached_to_vehicle);
        object_create.Write<std::uint16_t>(this_state.attach_info->attached_to_object);

        object_create.Write<float>(this_state.attach_info->attach_offset.x);
        object_create.Write<float>(this_state.attach_info->attach_offset.y);
        object_create.Write<float>(this_state.attach_info->attach_offset.z);
        object_create.Write<float>(this_state.attach_info->attach_rotation.x);
        object_create.Write<float>(this_state.attach_info->attach_rotation.y);
        object_create.Write<float>(this_state.attach_info->attach_rotation.z);
        object_create.Write<std::uint8_t>(this_state.attach_info->sync_rotation);
      }
      else {
        object_create.Write<std::uint16_t>(0xFFFF);
        object_create.Write<std::uint16_t>(0xFFFF);
      }
      object_create.Write<std::uint8_t>(this_state.materials.size());
      for (auto& mat : this_state.materials) {
        std::visit([&object_create]<typename T>(T& param) {
          if constexpr (std::is_same_v<T, Object::MaterialText>) {
            auto fontname = utf2cp(param.font_name);
            auto text = utf2cp(param.text);

            object_create.Write<std::uint8_t>(2);
            object_create.Write<std::uint8_t>(param._this_id);
            object_create.Write<std::uint8_t>(param.material_size);
            object_create.Write<std::uint8_t>(fontname.size());
            object_create.Write(fontname.c_str(), fontname.size());
            object_create.Write<std::uint8_t>(param.font_size);
            object_create.Write<std::uint8_t>(param.bold);
            object_create.Write<std::uint32_t>(param.font_color);
            object_create.Write<std::uint32_t>(param.background_color);
            object_create.Write<std::uint8_t>(param.align);
            StringCompressor::Instance()->EncodeString(text.c_str(), 2048,
                                                       reinterpret_cast<RakNet::BitStream*>(&object_create));
          }
          else if constexpr (std::is_same_v<T, Object::Material>) {
            auto& libname = param.libname;
            auto& texname = param.texname;

            object_create.Write<std::uint8_t>(1);
            object_create.Write<std::uint8_t>(param._this_id);
            object_create.Write<std::uint16_t>(param.model_id);
            object_create.Write<std::uint8_t>(libname.size());
            object_create.Write(libname.c_str(), libname.size());
            object_create.Write<std::uint8_t>(texname.size());
            object_create.Write(texname.c_str(), texname.size());
            object_create.Write<std::uint32_t>(param.color);
          }
        }, mat._this_object);

        material_info[this_state._this_id].emplace_back(std::move(mat));
      }

      samp_utils::impl_emul_rpc_bs(version, RPCEnumeration::RPC_ScrCreateObject, object_create);

      object_pool->Get(this_state._this_id)->m_pGameEntity->Teleport(this_state.position, false);
    }
  }

private:
  void on_rpc(RPCEnumeration rpc_id, BitStream* bs) {
    auto read_material = [this](BitStream* bs, std::uint16_t object_id, bool force_emplace) {
      std::uint8_t type;
      std::uint8_t id;
      bs->Read<std::uint8_t>(type);
      bs->Read<std::uint8_t>(id);

      auto& info = material_info[object_id];

      auto set_or_emplace = [&info](auto& mat, std::uint8_t id) {
        for (auto& el : info) {
          bool result = std::visit([id](auto& param) {
            if (param._this_id == id) {
              return true;
            }
            return false;
          }, el._this_object);

          if (result) {
            el._this_object = mat;
            break;
          }
        }
        info.emplace_back(std::move(mat));
      };

      if (type == 2) {
        char temp_buf[2048];

        Object::MaterialText this_material{};

        std::uint8_t fontname_len;

        bs->Read<std::uint8_t>(this_material.material_size);
        bs->Read<std::uint8_t>(fontname_len);
        this_material.font_name.resize(fontname_len);
        bs->Read(this_material.font_name.data(), fontname_len);
        bs->Read<std::uint8_t>(this_material.font_size);
        bs->Read<std::uint8_t>(this_material.bold);
        bs->Read<std::uint32_t>(this_material.font_color);
        bs->Read<std::uint32_t>(this_material.background_color);
        bs->Read<std::uint8_t>(this_material.align);

        StringCompressor::Instance()->DecodeString(temp_buf, 2048, reinterpret_cast<RakNet::BitStream*>(bs));

        this_material.text.assign(temp_buf, strlen(temp_buf));
        this_material.text = cp2utf(this_material.text);

        if (force_emplace)
          info.emplace_back(std::move(this_material));
        else
          set_or_emplace(this_material, id);
      }
      else if (type == 1) {
        std::uint8_t libname_len;
        std::uint8_t texname_len;

        Object::Material this_material{};
        this_material._this_id = id;
        bs->Read<std::uint16_t>(this_material.model_id);
        bs->Read<std::uint8_t>(libname_len);
        this_material.libname.resize(libname_len);
        bs->Read(this_material.libname.data(), libname_len);
        bs->Read<std::uint8_t>(texname_len);
        this_material.texname.resize(texname_len);
        bs->Read(this_material.texname.data(), texname_len);
        bs->Read<std::uint32_t>(this_material.color);

        if (force_emplace)
          info.emplace_back(std::move(this_material));
        else
          set_or_emplace(this_material, id);
      }
    };

    if (rpc_id == RPCEnumeration::RPC_ScrCreateObject) {
      std::uint16_t object_id;
      bs->Read<std::uint16_t>(object_id);

      bs->IgnoreBits(sizeof(float) * 7 * 8 + 32 + 8);
      std::uint16_t attached_to_vehicle;
      std::uint16_t attached_to_object;

      bs->Read<std::uint16_t>(attached_to_vehicle);
      bs->Read<std::uint16_t>(attached_to_object);
      if (attached_to_object != 0xFFFF || attached_to_vehicle != 0xFFFF) {
        bs->IgnoreBits(sizeof(float) * 6 * 8 + 8);
      }
      std::uint8_t tex_count;
      bs->Read<std::uint8_t>(tex_count);

      for (auto i = 0u; i < tex_count; ++i) {
        read_material(bs, object_id, true);
      }
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrSetObjectMaterial) {
      std::uint16_t object_id;
      bs->Read<std::uint16_t>(object_id);

      read_material(bs, object_id, false);
    }
    else if (rpc_id == RPCEnumeration::RPC_ScrDestroyObject) {
      std::uint16_t object_id;
      bs->Read<std::uint16_t>(object_id);
      material_info[object_id].clear();
    }

    bs->ResetReadPointer();
  }


  std::array<std::vector<Object::MaterialUnion>, 1000> material_info{};
};
