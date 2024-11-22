#include "doomgeneric.h"

#include <stdbool.h>
#include <stdio.h>

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

__attribute__((import_module("loading"))) void
getWadsSizes(int *numberOfWads, size_t *numberOfTotalBytesInAllWads);
__attribute__((import_module("loading"))) void
readDataForAllWads(unsigned char *wadDataDestination, int *byteLengthOfEachWad);

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

__attribute__((visibility("default"))) void initGame();
__attribute__((visibility("default"))) void tickGame();
__attribute__((visibility("default"))) void reportKeyDown(uint8_t doomKey);
__attribute__((visibility("default"))) void reportKeyUp(uint8_t doomKey);

// *****************************************************************************
// *                           IMPLEMENTATION DETAILS                          *
// *****************************************************************************

// A global cache of which keys are currently pressed down, according to what
// the user of this module has reported to via `reportKeyDown` and
// `reportKeyUp`.
static bool isKeyPressed[UINT8_MAX] = {false};

////////////////////////////////////////////////////////////////////////////////
// Here we implement the doomgeneric interface via the functions imported via
// WebAssembly imports.
////////////////////////////////////////////////////////////////////////////////

void DG_Init() { onGameInit(DOOMGENERIC_RESX, DOOMGENERIC_RESY); }

struct DB_BytesForAllWads DG_GetWads() {
  struct DB_BytesForAllWads result = {0};

  int numberOfWads;
  size_t numberOfTotalBytesInAllWads;
  getWadsSizes(&numberOfWads, &numberOfTotalBytesInAllWads);

  if (numberOfWads > 0) {
    unsigned char *wadData = malloc(numberOfTotalBytesInAllWads);
    int byteLengthOfEachWad[numberOfWads];
    readDataForAllWads(wadData, byteLengthOfEachWad);

    unsigned char *dataForNextWad = wadData;

    // Assign data for IWAD (which must be the first WAD)
    result.iWad.data = dataForNextWad;
    result.iWad.byteLength = byteLengthOfEachWad[0];
    dataForNextWad += byteLengthOfEachWad[0];

    // Assign data each PWADs
    int numberOfPWads = numberOfWads - 1;
    result.numberOfPWads = numberOfPWads;
    result.pWads = malloc(sizeof(struct DG_WadFileBytes) * numberOfPWads);
    for (int i = 0; i < numberOfPWads; i++) {
      result.pWads[i].data = dataForNextWad;
      result.pWads[i].byteLength = byteLengthOfEachWad[i + 1];
      dataForNextWad += byteLengthOfEachWad[i + 1];
    }
  } else {
    fprintf(
        stderr,
        "At least one WAD must be provided, but `getWadsSizes` reported 0.\n");
  }

  return result;
}

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

////////////////////////////////////////////////////////////////////////////////
// Here we implement, via the doomgeneric interface, the WebAssembly exported
// functions.
////////////////////////////////////////////////////////////////////////////////

void initGame() {
  // Provide 0 command line arguments to Doom.
  // Any configuration knobs we end up exposing via WebAssembly that would
  // usually be handled by command line arguments will instead be handled in a
  // different way.
  int argc = 0;
  char *argv[] = {};

  doomgeneric_Create(argc, argv);
}

void tickGame() { doomgeneric_Tick(); }

void reportKeyDown(uint8_t doomKey) { isKeyPressed[doomKey] = true; }

void reportKeyUp(uint8_t doomKey) { isKeyPressed[doomKey] = false; }
