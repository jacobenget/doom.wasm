#include <SDL.h>

#include "user_input.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(*array))

static struct {
  SDL_KeyCode sdlKeyCode;
  const char *doomKeyName;
} doomKeyNameForSdlKeyCodes[] = {
    {SDLK_LALT, "KEY_ALT"},
    {SDLK_RALT, "KEY_ALT"},
    {SDLK_BACKSPACE, "KEY_BACKSPACE"},
    {SDLK_DOWN, "KEY_DOWNARROW"},
    {SDLK_RETURN, "KEY_ENTER"},
    {SDLK_ESCAPE, "KEY_ESCAPE"},
    {SDLK_LCTRL, "KEY_FIRE"},
    {SDLK_RCTRL, "KEY_FIRE"},
    {SDLK_LEFT, "KEY_LEFTARROW"},
    {SDLK_RIGHT, "KEY_RIGHTARROW"},
    {SDLK_LSHIFT, "KEY_SHIFT"},
    {SDLK_RSHIFT, "KEY_SHIFT"},
    {SDLK_COMMA, "KEY_STRAFE_L"},
    {SDLK_PERIOD, "KEY_STRAFE_R"},
    {SDLK_TAB, "KEY_TAB"},
    {SDLK_UP, "KEY_UPARROW"},
    {SDLK_SPACE, "KEY_USE"},
};

bool pollUserInput(input_event_t *eventOut,
                   state_for_reading_global_constants_t *state,
                   read_global_constant_i32 *read_global_const) {

  // Loop until we either find an SDL event type we care about, or we run out of
  // pending events.
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      atexit(SDL_Quit);
      eventOut->eventType = EXIT;
      return true;

    } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
      int32_t doomKey = e.key.keysym.sym;

      for (int i = 0; i < ARRAY_LENGTH(doomKeyNameForSdlKeyCodes); i++) {
        if (sdlKeyCodesWithAssociatedDoomKeyName[i].sdlKeyCode ==
            e.key.keysym.sym) {
          doomKey = read_global_const(state,
                                      doomKeyNameForSdlKeyCodes[i].doomKeyName);
          break;
        }
      }

      eventOut->eventType = KEY_STATE_CHANGE;
      eventOut->keyStateChange.doomKey = doomKey;
      eventOut->keyStateChange.newState =
          (e.type == SDL_KEYDOWN) ? KEY_IS_DOWN : KEY_IS_UP;
      return true;
    }
  }

  return false;
}
