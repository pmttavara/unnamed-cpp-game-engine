#pragma once
#include "general.h"
#include "weapon.h"
#include "world.h"
struct Camera;
struct Input_State;
/* singleton */ struct Player {
    int health = 0;
    int max_health = 0;
    Bool alive() { return health > 0; }
    Vector2 pos = {}, vel = {}, accel = {};
    float friction = 0.f;
    Timer damage_invincibility = {};
    u16 location = 0xFFFF;
    //
    Bool transitioning_rooms = False;
    Room_Exit room_exit_info = {};
    Timer room_transition_timer = {
        0._s, world->room_fade_out_length +
                  world->room_fade_pause_length}; // @Migrate to Engine_State
    //
    ng::array<Weapon> arsenal = {world_allocator};
    usize current_weapon = 0;
    Bool jump_powerup = False;
    Timer jump_pre_input = {};  // When A is pressed just before landing
    Timer jump_post_input = {}; // When A is pressed just after falling
    struct CharacterController {
        f32 fallingGravity;
        f32 jumpingGravity; // \brief Used while jump button held
        f32 jumpVel;        // \brief Imparted when jump button pressed
        f32 walkSpeed;
        f32 walkAccel;
        f32 turnAccel;
        f32 friction; // \brief Velocity removed each second while grounded
        f32 airSpeed;
        f32 airAccel;
        f32 airTurn;
        f32 airDrag;
        f32 jump_pre_time;  // \brief Time to accept jump input before landing
        f32 jump_post_time; // \brief Time to accept jump input after leaving
                            // ground
        f32 jump_release_force;
    } ctrl = {};
    struct MovementState {
        // @Todo: remove all this OO get/setter garbage
        Bool Grounded() { return ground == GroundState::Grounded; }
        Bool Jumping() { return ground == GroundState::Jumping; }
        Bool Falling() { return ground == GroundState::Falling; }
        void Land() { ground = GroundState::Grounded; }
        void Jump() { ground = GroundState::Jumping; }
        void Fall() { ground = GroundState::Falling; }
        Bool FacingRight() { return facing == Facing::Right; }
        Bool FacingLeft() { return facing == Facing::Left; }
        void FaceRight() { facing = Facing::Right; }
        void FaceLeft() { facing = Facing::Left; }
        Bool stunned = False;

        enum struct GroundState {
            Grounded,
            Jumping,
            Falling
        } ground = GroundState::Falling;
        enum struct Facing { Right, Left } facing = Facing::Right;
    } mstate = {};
    Bool dead = False;
    Rect hitbox = {}; // \brief Collides with enemies, NPCs, items etc.
    Bool wants_to_interact = False;
    void init();
    void HandleInput(Input_State *input);
    void update(f32 dt, Room *room);
    void Jump();
    void Fall();
    void Die();
    void take_damage(u16 damage);
    void TakeExit(Room_Collision_Info *info);
    // camera stuff
    Vector2 focus_target = {};
    void target_me(Camera *camera);
    void update_focus_target();
    // audiovisuals
    Sprite sprite = {};
    Sprite eyes = {};
    Timer bleed_timer = {0.0_s, 0.5_s};
    ng::array<Sound> sounds = {audio_allocator};
    Sound jump_sound = {};
    Sound land_sound = {};
    void UpdateSprites();
    void Render(Camera *camera);
    Bool do_jump_release = False; // @Hack @Remove
    // collision
    struct Colliders {
        Rect x;
        Rect y;
    } colliders = {}; // \brief ng_for colliding with world

    // Input stuff. I'm starting out with direct maps to the keys.
    Vector2 in_vec = {};
    Bool a = False, a_new = False;
    Bool b = False, b_new = False;
    Bool x = False, x_new = False;
    Bool y = False, y_new = False;
    Bool down = False, down_new = False;

    Bool restarting = False; // transition after death
};
void player_destroy(Player *player);
Player player_copy(Player *player);
extern Player *player, *player_sbs;
