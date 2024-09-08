#include "doomgeneric.h"

#include <stdbool.h>

/*
 * This file provides augmentations to the interface (i.e. imports and exports)
 * that is produced when Doom is compiled to a WebAssembly module.
 */

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

__attribute__((import_module("ui"))) void onGameInit(int width, int height);
__attribute__((import_module("ui"))) void drawFrame(uint32_t *screenBuffer);
__attribute__((import_module("ui"))) void setWindowTitle(const char *title);

__attribute__((import_module("runtimeControl"))) void sleepMs(uint32_t ms);
__attribute__((import_module("runtimeControl"))) uint32_t getTicksMs();

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

// A global cache of which keys are currently pressed down, according to what
// the user of this module has reported to via `reportKeyDown` and
// `reportKeyUp`.
static bool isKeyPressed[UINT8_MAX] = {false};

__attribute__((visibility("default"))) void initGame(int argc, char **argv) {
  doomgeneric_Create(argc, argv);
}

__attribute__((visibility("default"))) void tickGame() { doomgeneric_Tick(); }

__attribute__((visibility("default"))) void reportKeyDown(uint8_t doomKey) {
  isKeyPressed[doomKey] = true;
}

__attribute__((visibility("default"))) void reportKeyUp(uint8_t doomKey) {
  isKeyPressed[doomKey] = false;
}

// *****************************************************************************
// *                           IMPLEMENTATION DETAILS                          *
// *****************************************************************************

// Here we implement the doomgeneric interface via the functions imported via
// WebAssembly imports.

void DG_Init() { onGameInit(DOOMGENERIC_RESX, DOOMGENERIC_RESY); }

void DG_DrawFrame() { drawFrame(DG_ScreenBuffer); }

int DG_GetKey(int *pressed, uint8_t *doomKey) {
  // Keep a cache of what the last communicated state of each key was, so that
  // each time the Doom engine calls this function we're prepared to relay any
  // key state changes that have been reported since the last call to this
  // function. By default all the keys start in the "not pressed" state.
  static bool statePreviouslyCommunicated[UINT8_MAX] = {false};

  for (int i = 0; i < UINT8_MAX; i++) {
    // If the actual state of this key doesn't match the last communicated state
    if (isKeyPressed[i] != statePreviouslyCommunicated[i]) {
      *pressed = isKeyPressed[i];
      *doomKey = i;
      // We're communicating this key state change, so update the cache of the
      // last communicated state of this key.
      statePreviouslyCommunicated[i] = isKeyPressed[i];
      return 1;
    }
  }
  return 0;
}

void DG_SetWindowTitle(const char *title) { setWindowTitle(title); }

void DG_SleepMs(uint32_t ms) { sleepMs(ms); }

uint32_t DG_GetTicksMs() { return getTicksMs(); }
