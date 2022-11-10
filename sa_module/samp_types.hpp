#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <variant>
#include <vector>
#include <string>
#include <CWeapon.h>

#include "quaternion.hpp"
#include "json_utils.hpp"
#include "vector.hpp"

#ifdef __RESHARPER__
#define WRAP_VERTAG sampapi::v037r1::VersionTag
//#define WRAP_VERTAG VerTag
#else
#define WRAP_VERTAG VerTag
#endif

#pragma pack(push, 1) // samp compatibility shit
struct Textdraw {
  union {
    struct {
      unsigned char box : 1;
      unsigned char left : 1;
      unsigned char right : 1;
      unsigned char center : 1;
      unsigned char proportional : 1;
    };

    unsigned char flags;
  };

  float letter_width{};
  float letter_height{};
  unsigned letter_color{};
  float box_width{};
  float box_height{};
  unsigned box_color{};
  unsigned char shadow{};
  bool outline{};
  unsigned back_color{};
  unsigned char style{};
  unsigned char is_selectable{};
  float x{};
  float y{};
  unsigned short model{};
  Vector rotation;
  float zoom{};
  unsigned short color[2]{};

  std::string text;
  unsigned short _this_id{};
};

struct ActorInfo {
  std::uint16_t id;
  int model;
  Vector position;
  float rotation;
  float health;
  bool invulnerable;
};
#pragma pack(pop)

struct TextdrawSelecion {
  int active;
  unsigned hover_color;
};

struct Vehicle {
  std::uint16_t _this_id;
  bool _is_automobile;

  std::uint32_t model_id;
  Vector spawn_pos;
  Vector pos;
  Quaternion rotation;
  std::uint8_t int_color1;
  std::uint8_t int_color2;
  float health;
  std::uint8_t interior_id;
  std::uint8_t siren;
  std::vector<std::uint8_t> mod_slots;
  std::uint8_t paintjob;
  std::uint32_t color1;
  std::uint32_t color2;
  std::int32_t doors_damage;
  std::int32_t panel_damage;
  std::int8_t lights_damage;
  std::int8_t tires_damage;

  std::uint8_t lock_doors;

  std::uint16_t trailer_id;

  std::string number_plate;
  std::uint16_t driver;
  std::array<std::uint16_t, 8> passengers;
};

struct RemoveBuilding {
  std::uint32_t model_id;
  Vector position;
  float radius;
};

struct Accesory {
  std::uint32_t _this_id;

  std::uint32_t model;
  std::uint32_t bone;
  Vector offset;
  Vector rotation;
  Vector scale;

  std::uint32_t first_color;
  std::uint32_t second_color;
};

struct ObjectAttachInfo {
  std::uint16_t object_id{0xFFFF};
  Vector offset;
  Vector rotation;
};

struct SkillInfo {
  std::uint32_t weapon_id;
  std::uint16_t skill_level;
};

struct Player {
  std::uint16_t _this_id;

  std::string nick;
  std::uint32_t color;
  std::uint8_t team;
  bool is_npc;
  bool show_nametags;
  bool spawned;

  std::uint32_t model;
  Vector position;
  std::uint8_t fightstyle;

  std::optional<ObjectAttachInfo> object_attach;
  std::vector<Accesory> accesories;
  std::vector<SkillInfo> skills;
};

struct Pickup {
  enum Type {
    kDefault,
    kWeapon,
  };

  std::int32_t model_id;
  std::int32_t type;
  Vector position;

  Type _this_type;
  std::uint16_t _this_id;

  std::uint32_t ammo;
  std::uint16_t ex_owner;
};

struct Object {
  struct Material {
    std::uint8_t _this_id{};

    std::uint16_t model_id{};
    std::string libname;
    std::string texname;

    std::uint32_t color{};
  };

  struct MaterialText {
    std::uint8_t _this_id{};

    std::uint8_t material_size{};

    std::uint8_t font_size{};
    std::uint8_t bold{};
    std::uint8_t align{};

    std::string font_name;
    std::uint32_t font_color{};
    std::uint32_t background_color{};
    std::string text;
  };

  struct MaterialUnion {
    std::variant<Material, MaterialText> _this_object;
  };

  std::uint16_t _this_id{};
  std::uint16_t attached_to_player{};

  std::uint32_t model_id{};
  float draw_distance{};
  Vector position;
  Vector rotation;

  struct AttachInfo {
    std::uint16_t attached_to_object{};
    std::uint16_t attached_to_vehicle{};
    Vector attach_offset;
    Vector attach_rotation;
    bool sync_rotation{};
  };

  std::optional<AttachInfo> attach_info;
  bool dont_collide_with_camera{};

  std::vector<MaterialUnion> materials;
};

struct ObjectEdit {
  int edit_type;
  int edit_mode;

  Vector rotation;
  std::array<std::array<float, 4>, 4> matrix;

  int process_type;
  std::uint32_t object_idx;
  bool is_player_object;
};

struct NetGameSettings {
  long long save_tick;
  unsigned int timer_ticks;
  bool m_bUseCJWalk;
  unsigned int m_nDeadDropsMoney;
  float m_fWorldBoundaries[4];
  bool m_bAllowWeapons;
  float m_fGravity;
  bool m_bEnterExit;
  sampapi::BOOL m_bVehicleFriendlyFire;
  bool m_bHoldTime;
  bool m_bInstagib;
  bool m_bZoneNames;
  bool m_bFriendlyFire;
  sampapi::BOOL m_bClassesAvailable;
  float m_fNameTagsDrawDist;
  bool m_bManualVehicleEngineAndLight;
  unsigned char m_nWorldTimeHour;
  unsigned char m_nWorldTimeMinute;
  unsigned char m_nWeather;
  bool m_bNoNametagsBehindWalls;
  int m_nPlayerMarkersMode;
  float m_fChatRadius;
  bool m_bNameTags;
  bool m_bLtdChatRadius;

  // NOTSAMP
  bool disable_collision;
  bool update_camera_target;
  bool stunt_bonus;
  bool lan_mode;
  unsigned short playerid;
  unsigned onfoot_sendrate;
  unsigned incar_sendrate;
  unsigned firing_sendrate;
  unsigned sendmult;
  unsigned lagcomp_mode;
};

struct LocalPlayer {
  struct Weapon {
    eWeaponType type;
    unsigned total_ammo;
  };

  struct SpawnInfo {
    std::uint8_t team;
    std::uint32_t model_id;
    std::uint8_t field_c;
    Vector position;
    float rotation;
    std::array<int, 3> weapon;
    std::array<int, 3> ammo;
  };

  struct SpectateInfo {
    std::int8_t mode;
    std::int8_t type;
    std::uint16_t object_id;
  };

  Vector position;
  Quaternion rotation;

  float heading;
  float health;
  float armour;
  float max_health;

  std::uint16_t _this_id;
  std::uint8_t armed_weapon;
  std::uint8_t interior_id;
  std::array<Weapon, 13> weapons;
  std::uint8_t wanted_level;
  bool is_controllable;
  bool is_spectating;
  bool goto_class_selection;
  bool in_class_selection;
  int class_selection_number;
  std::int32_t money;
  std::uint32_t drunk_level;

  std::uint8_t team;
  std::uint32_t color;
  std::uint32_t model_id;

  std::string nick;
  std::optional<std::string> shop_name;

  std::vector<SkillInfo> skills;

  SpawnInfo spawn_info;
  std::vector<Accesory> accesories;
  std::optional<SpectateInfo> spectate_info;
  std::optional<ObjectAttachInfo> object_attach;
};

struct Label {
  sampapi::ID this_id;

  std::string text;
  unsigned long color;
  Vector position;
  float draw_distance;
  bool draw_behind_walls;
  sampapi::ID attached_to_player;
  sampapi::ID attached_to_vehicle;
};

struct KillList {
  int enabled;

  struct Entry {
    std::string killer;
    std::string victim;
    unsigned killer_color;
    unsigned victim_color;
    char weapon;
  };

  std::array<Entry, 5> entries;
};

struct Marker {
  Vector position;
  std::uint8_t icon_id;
  std::int8_t type;
  std::uint8_t style;

  bool _enabled;
  std::uint32_t color;
};

struct Menu {
  struct Interaction {
    int menu;
    std::array<int, 12> row;
    std::array<int, 3> padding;
  };

  std::uint8_t _this_id;
  std::string title;
  std::array<std::vector<std::string>, 2> items;
  std::vector<std::string> headers;
  float pos_x;
  float pos_y;
  float first_column_width;
  float second_column_width;
  std::int8_t num_columns;
  Interaction interaction;

  std::array<std::uint8_t, 2> row_count;
};

struct GangZone {
  unsigned _this_id;

  float left;
  float bottom;
  float right;
  float top;
  unsigned color;
  unsigned alt_color;
};

struct Dialog {
  int id;
  int type;
  std::string caption;
  std::string text;
  std::string left_button;
  std::string right_button;

  bool serverside;
};

struct Checkpoints {
  struct Default {
    Vector position;
    Vector size;
  };

  struct Race {
    Vector current_position;
    Vector next_position;
    float size;
    std::uint8_t type;
  };

  std::optional<Default> checkpoint;
  std::optional<Race> race_checkpoint;
};

struct Chat {
  int mode;

  struct ChatEntry {
    std::int32_t timestamp{};

    std::string prefix;
    std::string text;
    int type{};
    unsigned text_color{};
    unsigned prefix_color{};
  };

  std::vector<ChatEntry> entries;
};

struct Camera {
  struct LookAt {
    Vector look_at;
    Vector position;
    Vector rotation;
  };

  struct MoveCam {
    bool track;
    bool with_ease;

    unsigned ticks_left;

    Vector from;
    Vector to;
  };

  std::uint16_t attached_to_object;
  std::optional<LookAt> look_at_coords;
  std::optional<MoveCam> move_cam;
};

struct StartupArgs {
  std::string nick;
  std::string hostname;
  std::string port;
  std::string password;

  friend bool operator==(const StartupArgs& lhs, const StartupArgs& rhs) {
    return lhs.nick == rhs.nick
      && lhs.hostname == rhs.hostname
      && lhs.port == rhs.port
      && lhs.password == rhs.password;
  }

  friend bool operator!=(const StartupArgs& lhs, const StartupArgs& rhs) {
    return !(lhs == rhs);
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StartupArgs,
                                   nick,
                                   hostname,
                                   port,
                                   password)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera::LookAt,
                                   look_at,
                                   position,
                                   rotation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera::MoveCam,
                                   with_ease,
                                   track,
                                   ticks_left,
                                   from,
                                   to)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera,
                                   attached_to_object,
                                   look_at_coords,
                                   move_cam)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActorInfo,
                                   id,
                                   model,
                                   position,
                                   rotation,
                                   health,
                                   invulnerable)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Chat::ChatEntry,
                                   timestamp,
                                   prefix,
                                   text,
                                   type,
                                   text_color,
                                   prefix_color)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Chat,
                                   entries,
                                   mode)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Checkpoints::Default,
                                   position,
                                   size)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Checkpoints::Race,
                                   current_position,
                                   next_position,
                                   size,
                                   type)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Checkpoints,
                                   checkpoint,
                                   race_checkpoint)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dialog,
                                   id,
                                   type,
                                   caption,
                                   text,
                                   left_button,
                                   right_button,
                                   serverside);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GangZone,
                                   _this_id,
                                   left,
                                   bottom,
                                   right,
                                   top,
                                   color,
                                   alt_color)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Menu::Interaction,
                                   menu,
                                   row,
                                   padding)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Menu,
                                   _this_id,
                                   title,
                                   items,
                                   headers,
                                   pos_x,
                                   pos_y,
                                   first_column_width,
                                   second_column_width,
                                   num_columns,
                                   interaction,
                                   row_count)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Marker,
                                   position,
                                   icon_id,
                                   type,
                                   style,
                                   color)


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KillList::Entry,
                                   killer,
                                   victim,
                                   killer_color,
                                   victim_color,
                                   weapon)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KillList,
                                   entries,
                                   enabled)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Label,
                                   this_id,
                                   text,
                                   color,
                                   position,
                                   draw_distance,
                                   draw_behind_walls,
                                   attached_to_player,
                                   attached_to_vehicle)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LocalPlayer::Weapon,
                                   type,
                                   total_ammo)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LocalPlayer::SpawnInfo,
                                   team,
                                   model_id,
                                   field_c,
                                   position,
                                   rotation,
                                   weapon,
                                   ammo)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LocalPlayer::SpectateInfo,
                                   mode,
                                   type,
                                   object_id)


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetGameSettings,
                                   disable_collision,
                                   m_bUseCJWalk,
                                   m_nDeadDropsMoney,
                                   m_fWorldBoundaries,
                                   m_bAllowWeapons,
                                   m_fGravity,
                                   m_bEnterExit,
                                   m_bVehicleFriendlyFire,
                                   m_bHoldTime,
                                   m_bInstagib,
                                   m_bZoneNames,
                                   m_bFriendlyFire,
                                   m_bClassesAvailable,
                                   m_fNameTagsDrawDist,
                                   m_bManualVehicleEngineAndLight,
                                   m_nWorldTimeHour,
                                   m_nWorldTimeMinute, m_nWeather,
                                   m_bNoNametagsBehindWalls,
                                   m_nPlayerMarkersMode,
                                   m_fChatRadius,
                                   m_bNameTags,
                                   m_bLtdChatRadius,
                                   stunt_bonus,
                                   lan_mode,
                                   playerid,
                                   onfoot_sendrate,
                                   incar_sendrate,
                                   firing_sendrate,
                                   sendmult,
                                   lagcomp_mode,
                                   update_camera_target,
                                   timer_ticks,
                                   save_tick)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectEdit,
                                   edit_type,
                                   edit_mode,
                                   rotation,
                                   matrix,
                                   process_type,
                                   object_idx,
                                   is_player_object)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Object::Material,
                                   _this_id,
                                   model_id,
                                   libname,
                                   texname,
                                   color)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Object::MaterialText,
                                   _this_id,
                                   material_size,
                                   font_size,
                                   bold,
                                   align,
                                   font_name,
                                   font_color,
                                   background_color,
                                   text)

inline void from_json(const nlohmann::json& j, Object::MaterialUnion& to) {
  if (auto type = j["this_type"].get<std::string>(); type == "text") {
    to._this_object = j["info"].get<Object::MaterialText>();
  }
  else if (type == "texture") {
    to._this_object = j["info"].get<Object::Material>();
  }
  else {
    // wtf
  }
}

inline void to_json(nlohmann::json& j, const Object::MaterialUnion& to) {
  std::visit([&j]<typename T>(T& param) {
    using decayed = std::decay_t<T>;
    if constexpr (std::is_same_v<decayed, Object::MaterialText>) {
      j["this_type"] = "text";
    }
    else if constexpr (std::is_same_v<decayed, Object::Material>) {
      j["this_type"] = "texture";
    }
    j["info"] = param;
  }, to._this_object);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Object::AttachInfo,
                                   attached_to_object,
                                   attached_to_vehicle,
                                   attach_offset,
                                   attach_rotation,
                                   sync_rotation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Object,
                                   _this_id,
                                   attached_to_player,
                                   model_id,
                                   draw_distance,
                                   position,
                                   rotation,
                                   attach_info,
                                   dont_collide_with_camera,
                                   materials)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pickup,
                                   _this_id,
                                   _this_type,
                                   model_id,
                                   type,
                                   position,
                                   ammo,
                                   ex_owner)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Accesory,
                                   _this_id,
                                   model,
                                   bone,
                                   offset,
                                   rotation,
                                   scale,
                                   first_color,
                                   second_color)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ObjectAttachInfo,
                                   object_id,
                                   offset,
                                   rotation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SkillInfo,
                                   weapon_id,
                                   skill_level)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LocalPlayer,
                                   _this_id,
                                   position,
                                   rotation,
                                   heading,
                                   health,
                                   armour,
                                   interior_id,
                                   max_health,
                                   armed_weapon,
                                   weapons,
                                   wanted_level,
                                   is_controllable,
                                   is_spectating,
                                   money,
                                   drunk_level,
                                   team,
                                   color,
                                   model_id,
                                   nick,
                                   skills,
                                   spawn_info,
                                   spectate_info,
                                   object_attach,
                                   accesories,
                                   goto_class_selection,
                                   shop_name,
                                   in_class_selection,
                                   class_selection_number)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Player,
                                   _this_id,
                                   nick,
                                   color,
                                   team,
                                   is_npc,
                                   show_nametags,
                                   spawned,
                                   model,
                                   position,
                                   fightstyle,
                                   accesories,
                                   object_attach)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RemoveBuilding,
                                   model_id,
                                   position,
                                   radius)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vehicle,
                                   _this_id,
                                   _is_automobile,
                                   model_id,
                                   spawn_pos,
                                   pos,
                                   rotation,
                                   health,
                                   int_color1,
                                   int_color2,
                                   interior_id,
                                   siren,
                                   mod_slots,
                                   paintjob,
                                   color1,
                                   color2,
                                   doors_damage,
                                   panel_damage,
                                   lights_damage,
                                   tires_damage,
                                   lock_doors,
                                   trailer_id,
                                   number_plate,
                                   driver,
                                   passengers
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Textdraw,
                                   flags,
                                   letter_width,
                                   letter_height,
                                   letter_color,
                                   box_width,
                                   box_height,
                                   box_color,
                                   shadow,
                                   outline,
                                   back_color,
                                   style,
                                   is_selectable,
                                   x,
                                   y,
                                   model,
                                   rotation,
                                   zoom,
                                   color,
                                   text,
                                   _this_id)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextdrawSelecion, active, hover_color)
