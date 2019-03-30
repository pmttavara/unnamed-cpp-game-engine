#pragma once
#include "renderer.h"
struct Room;
enum struct Enemy_Type : u8 {
  UNINITIALIZED,

  GOOMBER,
  SYNCED_JUMPER,
  FLYING_RAT,
  RUNNY_RABBIT,
  CANNON_TOAST,
  CHARGING_CHEETAH,

  NUM_ENEMY_TYPES
};
namespace ng {
inline void print_item(print_buffer *buf, Enemy_Type type) {
  static ng::string names[cast(int) Enemy_Type::NUM_ENEMY_TYPES] = {
    ""_s,
    "Goomber"_s,
    "Synced Jumper"_s,
    "Flying Rat"_s,
    "Runny Rabbit"_s,
    "Cannon Toast"_s,
    "Charging Cheetah"_s,
  };
  ng::print_item(buf, names[cast(int) type]);
}
}
struct Enemy_Data {
  Sprite sprite = {};
  Rect hitbox = {};
  Rect player_collider = {};
  f32 gravity = 0._g;
  u16 max_health = 0_hp;
  u16 damageplayer_amount = 0_hp;
  f32 response_radius = 0._g;
};
extern Enemy_Data enemy_data[cast(int) Enemy_Type::NUM_ENEMY_TYPES];
void init_enemy_data();
void deinit_enemy_data();
struct Enemy {
  Enemy_Type type = cast(Enemy_Type) 0;
  Bool ground = False;
  u16 location = 0xFFFF;
  int health = 0;
  int max_health = 0;
  Bool alive() { return health > 0; }
  Bool facing_left = False;
  Sprite sprite = {};
  Vector2 pos = {};
  Vector2 vel = {};
  Timer ai_ticker = {};

  union {
    struct {

    } goomber = {};
    struct {

    } synced_jumper;
    struct {

    } flying_rat;
    struct {
        Timer smoke_timer = {0.0_s, 0.02_s};
    } runny_rabbit;
    struct {

    } cannon_toast;
    struct {

    } charging_cheetah;
  };

  Enemy() : goomber{} {};
  Enemy(Enemy_Type type, u16 location, s16 x, s16 y);
  Enemy_Data *get_data();
  void take_damage(u16 damage, Vector2 knockback);
  void respond_to_player_jump(struct Player *player);
  void update(f32 dt, Room *room);
  void Render();
};
Enemy enemy_copy(Enemy *rhs);
void enemy_destroy(Enemy *rhs);
