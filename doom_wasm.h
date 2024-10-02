#ifndef DOOM_WASM_H_
#define DOOM_WASM_H_

#include <stddef.h>
#include <stdint.h>

#define IMPORT_MODULE(moduleName) __attribute__((import_module(moduleName)))
#define EXPORT __attribute__((visibility("default")))

/*
 * This file specifies the module interface (i.e. imports and exports) that is
 * produced when Doom is compiled to a WebAssembly module.
 */

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

EXPORT void initGame();
EXPORT void tickGame();
EXPORT void reportKeyDown(uint8_t doomKey);
EXPORT void reportKeyUp(uint8_t doomKey);

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

IMPORT_MODULE("loading") void onGameInit(int width, int height);
IMPORT_MODULE("loading")
void getWadsSizes(int *numberOfWads, size_t *numberOfTotalBytesInAllWads);
IMPORT_MODULE("loading")
void readDataForAllWads(unsigned char *wadDataDestination,
                        int *byteLengthOfEachWad);

IMPORT_MODULE("ui") void drawFrame(uint32_t *screenBuffer);
IMPORT_MODULE("ui") void setWindowTitle(const char *title); // TODO: remove this

IMPORT_MODULE("console") void info(const char *message, size_t length);
IMPORT_MODULE("console") void error(const char *message, size_t length);

IMPORT_MODULE("runtimeControl") void sleepMs(uint32_t ms);
IMPORT_MODULE("runtimeControl") uint32_t getTicksMs();
IMPORT_MODULE("runtimeControl") void onExit(int32_t exitCode);

// Return the size of the associated save game data, 0 in the case that no save
// data exists for this slot
IMPORT_MODULE("gameSaving") size_t sizeOfSaveGame(int gameSaveId);
// Return the number of bytes read
IMPORT_MODULE("gameSaving")
size_t readSaveGame(int gameSaveId, unsigned char *dataDestination);
// Return the number of bytes written
IMPORT_MODULE("gameSaving")
size_t writeSaveGame(int gameSaveId, unsigned char *data, size_t length);

#endif /* DOOM_WASM_H_ */
