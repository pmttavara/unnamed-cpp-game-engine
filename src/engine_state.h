// @Remove
#pragma once
#include "general.h"
enum struct Engine_Mode {
  MAIN_MENU,
  PAUSED,
  TRANSITION_FREEZE,
  IN_ENGINE,
  IN_EDITOR,
};
struct Engine_State { // @Cleanup: move all global state into a Globals struct
  Engine_Mode mode = Engine_Mode::MAIN_MENU;
  Bool should_quit = False;
  Bool game_data_loaded = False;
  Bool savegame_loaded = False;
};
extern Engine_State engine_state;
