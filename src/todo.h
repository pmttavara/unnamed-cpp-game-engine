/*
* Make a Profile_Counter struct+macro in general.h
* De-RAII ng::array (do this by removing the destructor, then checking every usage site for when to copy and when to release.)
* Bug: The player's eye sprite appears delayed 1 frame behind the main sprite when
    he is running and bobbing his head. It is particularly noticeable at low framerates.
    Horizontal position is correct, vertical offset is wrong. Something to do with UpdateSprites()?
 -> Fixed.
* Bug: UI interaction prompts still render after returning to the main menu.
 -> Fixed.
* Rip out all the time tick stuff and switch to: const f64 dt = fps_to_s(update_rate); for(;;) { game_tick(dt); SDL_Delay(s_to_ms(dt)); }
* Use separate RNGs for gameplay and FX.
* Reset RNGs every time the game is restarted. (Not the executable.)
* Inline all the ng::abs functions. Some are outlined but they probably can't be made intrinsics so it's pessimal.
* Weird bug: After switching ng::print back to taking const char *, the profiler's alignment is off. Probably missing a char or something.
 -> Fixed.
* In ng.hpp, make an auto_array that is just an ng::array whose destructor calls release().



- Move all global state to Globals.
- Bug: Text box renders at incorrect position on the screen.
- Fix all the awful memory leaks everywhere. (Something something OOP solves everything)
- Gamepad support for the game.
- Turn game units into something other than pixel units, and flip the Y axis.
- In ng.hpp, rename all format_* to fmt* or something.


- @@@@@@@@@@@@@@@@@@@@@@@@@@@@@
- @ MOST IMPORTANT: PLAN GAME @
- @@@@@@@@@@@@@@@@@@@@@@@@@@@@@

- Goal: Get the game done
    => Get all the content created
        => Get the mechanics nailed down (asap!)
            => Construct more better testing environments
                => Make the world-editor easy to use and fully capable
                    => Finish the missing features
                        => Give the editor the ability to modify Music_Info, Exits, the ID, the filename, the layer textures...
                            => Exits first
                                => Finish code to visually render exits in the editor
                                    => Write functional UI code
        => Design levels
            => Idea: make many sets of levels, order them by difficulty, then connect them all together into one big world
                => Make the world-editor easy to use
                    => Give the editor the ability to modify enemy locations, pickup placement...
        => Plan out story
        => Do artwork for different level themes





*/
