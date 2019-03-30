#pragma once
#include "renderer.h"
#include "audio.h"
struct Player;
struct Enemy;
using enemy_list_t = ng::array<Enemy>;
enum struct Weapon_Type : u8 {
  FICKLE_SICKLE,
  VINE_WHIP,
  LIL_DRILL,
  CRYSTAL_CANTOR,
  MANDIBLE_MANDOBLE,
  UNNAMED_GUN,

  NUM_WEAPON_TYPES
};
struct Weapon_Data {
  Sprite sprite = {};
  Rect hitbox = {};
  // \brief Amount of knockback this applies to enemies.
  Vector2 enemy_knockback = {};
  // \brief Info for lifting the player when swinging a melee weapon.
  // \brief Added to vertical velocity on swing.
  s16 lift_amount = 0;
  // \brief Amount of damage this weapon deals.
  u16 damage = 0_hp;
  Sound hit_sound = {};
  Sound swing_sound = {};
  enum struct Trigger_Mode : u8 { TAP, HOLD } mode = Trigger_Mode::TAP;
  enum struct Attack_Type : u8 { MELEE, RANGED } type = Attack_Type::MELEE;
  f32 swing = 0.0_s;
  f32 cooldown = 0.0_s;
  struct {
    Texture texture = {};
    Rect hitbox = {};
    Vector2 vel = {};
  } projectile = {};
};
extern Weapon_Data weapon_data[cast(int) Weapon_Type::NUM_WEAPON_TYPES];
void init_weapon_data();
void deinit_weapon_data();
struct Weapon {
  Weapon_Type type = cast(Weapon_Type) 0;
  enum struct State : u8 {
    Inactive,
    Cooldown,
    Swinging
  } state = State::Inactive;
  Sprite sprite = {};
  // \brief ng_for cooldown timer, as well as swing if in melee mode.
  Timer timer = {};
  Player *owner = null;
  void update(f32 dt);
  void Render(Vector2 offset = {});
  void Attack();
  Rect getAABB();

private:
  // \brief "Hit" as in actually do the "attack" part of the attack, rather than
  // just the swing part.
  void Hit();
};
Weapon_Data *weapon_get_data(Weapon_Type type);
Weapon weapon_from_type(Weapon_Type type, Player *owner);
Weapon weapon_copy(Weapon *rhs);
void weapon_destroy(Weapon *wep);
// static_assert(sizeof(Weapon) == 16, "");
