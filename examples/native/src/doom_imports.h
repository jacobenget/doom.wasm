
#ifndef DOOM_IMPORTS_H_
#define DOOM_IMPORTS_H_

// IMPORTS

// function console.onErrorMessage(i32, i32) -> ()
// Needs access to memory
void console_onErrorMessage(int32_t messagePtr, int32_t length,
                            uint8_t *memory);

// function console.onInfoMessage(i32, i32) -> ()
// Needs access to memory
void console_onInfoMessage(int32_t messagePtr, int32_t length, uint8_t *memory);

// function gameSaving.readSaveGame(i32, i32) -> (i32)
// Needs access to memory
int32_t gameSaving_readSaveGame(int32_t gameSaveId, int32_t dataDestinationPtr,
                                uint8_t *memory);

// function gameSaving.sizeOfSaveGame(i32) -> (i32)
int32_t gameSaving_sizeOfSaveGame(int32_t gameSaveId);

// function gameSaving.writeSaveGame(i32, i32, i32) -> (i32)
// Needs access to memory
int32_t gameSaving_writeSaveGame(int32_t gameSaveId, int32_t dataPtr,
                                 int32_t length, uint8_t *memory);

// function loading.onGameInit(i32, i32) -> ()
void loading_onGameInit(int32_t width, int32_t height);

// function loading.readWads(i32, i32) -> ()
// Needs access to memory
void loading_readWads(int32_t wadDataDestinationPtr,
                      int32_t byteLengthOfEachWadPtr, uint8_t *memory);

// function loading.wadSizes(i32, i32) -> ()
void loading_wadSizes(int32_t numberOfWadsPtr,
                      int32_t numberOfTotalBytesInAllWadsPtr);

// function runtimeControl.timeInMilliseconds() -> (i64)
int64_t runtimeControl_timeInMilliseconds();

// function ui.drawFrame(i32) -> ()
// Needs access to memory
void ui_drawFrame(int32_t screenBufferPtr, uint8_t *memory);

#endif
