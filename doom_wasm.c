#include "doomgeneric.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * This file provides augmentations to the interface (i.e. imports and exports)
 * that is produced when Doom is compiled to a WebAssembly module.
 */

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

#define IMPORT_MODULE(moduleName) __attribute__((import_module(moduleName)))

IMPORT_MODULE("loading") void onGameInit(int width, int height);
IMPORT_MODULE("loading")
void getWadsSizes(int *numberOfWads, size_t *numberOfTotalBytesInAllWads);
IMPORT_MODULE("loading")
void readDataForAllWads(unsigned char *wadDataDestination,
                        int *byteLengthOfEachWad);

IMPORT_MODULE("ui") void drawFrame(uint32_t *screenBuffer);
IMPORT_MODULE("ui") void setWindowTitle(const char *title);

IMPORT_MODULE("runtimeControl") void sleepMs(uint32_t ms);
IMPORT_MODULE("runtimeControl") uint32_t getTicksMs();

// Return the size of the associated save game data, 0 in the case that no save
// data exists for this slot
IMPORT_MODULE("gameSaving") size_t sizeOfSaveGame(int gameSaveId);
// Return the number of bytes read
IMPORT_MODULE("gameSaving")
size_t readSaveGame(int gameSaveId, unsigned char *dataDestination);
// Return the number of bytes written
IMPORT_MODULE("gameSaving")
size_t writeSaveGame(int gameSaveId, unsigned char *data, size_t length);

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

#define EXPORT __attribute__((visibility("default")))

EXPORT void initGame();
EXPORT void tickGame();
EXPORT void reportKeyDown(uint8_t doomKey);
EXPORT void reportKeyUp(uint8_t doomKey);

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

//
// Features related to the reading and writing of saved games
//

typedef struct {
  save_game_reader_t reader;
  unsigned char *buffer;
  size_t length;
  size_t offset;
} buffer_save_game_reader_t;

typedef struct {
  save_game_writer_t writer;
  unsigned char *buffer;
  size_t length;
  size_t offset;
  int saveGameSlot;
} buffer_save_game_writer_t;

//
// Implement the functions that make up save_game_reader_t for when the data is
// being read directly from a buffer.
//

static size_t SaveGameReader_ReadBytes(struct save_game_reader *reader,
                                       unsigned char *destination,
                                       size_t numberOfBytes) {
  buffer_save_game_reader_t *bufferReader = (buffer_save_game_reader_t *)reader;
  memcpy(destination, bufferReader->buffer + bufferReader->offset,
         numberOfBytes);
  bufferReader->offset += numberOfBytes;
  return numberOfBytes;
}

static long SaveGameReader_BytesReadSoFar(struct save_game_reader *reader) {
  buffer_save_game_reader_t *bufferReader = (buffer_save_game_reader_t *)reader;
  return bufferReader->offset;
}

static int SaveGameReader_Close(struct save_game_reader *reader) {
  buffer_save_game_reader_t *bufferReader = (buffer_save_game_reader_t *)reader;
  free(bufferReader->buffer);
  free(bufferReader);
  return 0;
}

//
// Implement the functions that make up save_game_writer_t for when the data is
// being read directly from a buffer.
//

static size_t SaveGameWriter_WriteBytes(struct save_game_writer *writer,
                                        unsigned char *source,
                                        size_t numberOfBytes) {
  buffer_save_game_writer_t *bufferWriter = (buffer_save_game_writer_t *)writer;

  // If the current buffer isn't big enough to hold the data we wish to write,
  // increase the buffer size so it's big enough
  size_t lengthNeeded = bufferWriter->offset + numberOfBytes;
  if (lengthNeeded > bufferWriter->length) {
    size_t newLength =
        (bufferWriter->length * 3) / 2; // increase buffer size by 50%
    if (lengthNeeded > newLength) {
      newLength = lengthNeeded;
    }

    unsigned char *biggerBuffer = malloc(newLength);
    if (!biggerBuffer) {
      return 0;
    }
    memcpy(biggerBuffer, bufferWriter->buffer, bufferWriter->offset);
    free(bufferWriter->buffer);
    bufferWriter->buffer = biggerBuffer;
    bufferWriter->length = newLength;
  }

  memcpy(bufferWriter->buffer + bufferWriter->offset, source, numberOfBytes);
  bufferWriter->offset += numberOfBytes;

  return numberOfBytes;
}

static long SaveGameWriter_BytesWrittenSoFar(struct save_game_writer *writer) {
  buffer_save_game_writer_t *bufferWriter = (buffer_save_game_writer_t *)writer;
  return bufferWriter->offset;
}

static int SaveGameWriter_Close(struct save_game_writer *writer) {

  buffer_save_game_writer_t *bufferWriter = (buffer_save_game_writer_t *)writer;

  size_t sizeWritten = writeSaveGame(
      bufferWriter->saveGameSlot, bufferWriter->buffer, bufferWriter->length);

  if (bufferWriter->buffer) {
    free(bufferWriter->buffer);
  }
  free(bufferWriter);

  if (sizeWritten != bufferWriter->length) {
    return EOF;
  } else {
    return 0;
  }
}

// Return NULL if there is no save game data saved to this slot
save_game_reader_t *DG_OpenSaveGameReader(int saveGameSlot) {
  size_t saveGameSize = sizeOfSaveGame(saveGameSlot);

  if (!saveGameSize) {
    return NULL;
  }

  unsigned char *buffer = malloc(saveGameSize);

  if (!buffer) {
    return NULL;
  }

  size_t bytesRead = readSaveGame(saveGameSlot, buffer);

  if (bytesRead != saveGameSize) {
    free(buffer);
    return NULL;
  }

  buffer_save_game_reader_t *bufferReader =
      malloc(sizeof(buffer_save_game_reader_t));
  bufferReader->reader.ReadBytes = SaveGameReader_ReadBytes;
  bufferReader->reader.BytesReadSoFar = SaveGameReader_BytesReadSoFar;
  bufferReader->reader.Close = SaveGameReader_Close;
  bufferReader->buffer = buffer;
  bufferReader->length = saveGameSize;
  bufferReader->offset = 0;

  return &bufferReader->reader;
}

save_game_writer_t *DG_OpenSaveGameWriter(int saveGameSlot) {
  // A few save game file examples that were tested were around 25k bytes,
  // so we'll start with an initial buffer size just a bit more than that.
  const size_t initialSaveGameBufferSize = 30000;
  unsigned char *buffer = malloc(initialSaveGameBufferSize);

  if (!buffer) {
    return NULL;
  }

  buffer_save_game_writer_t *bufferWriter =
      malloc(sizeof(buffer_save_game_writer_t));
  bufferWriter->writer.WriteBytes = SaveGameWriter_WriteBytes;
  bufferWriter->writer.BytesWrittenSoFar = SaveGameWriter_BytesWrittenSoFar;
  bufferWriter->writer.Close = SaveGameWriter_Close;
  bufferWriter->buffer = buffer;
  bufferWriter->length = initialSaveGameBufferSize;
  bufferWriter->offset = 0;
  bufferWriter->saveGameSlot = saveGameSlot;

  return &bufferWriter->writer;
}

void DG_DemoRecorded(const char *demoName, unsigned char *demoBytes,
                     size_t demoSize) {
  // Do nothing for now in response to a demo being recorded, because we don't
  // currently support trigging the recording of a demo via the WebAssembly, so
  // we know this function should never be called.
  //
  // Recording a demo in Doom is entirely done by passing the "-record" command
  // line argument to Doom, which we don't currently allow in this
  // implementation (in fact, we don't allow the passing of ANY command line
  // arguments to Doom).
}

void DG_PCXScreenshotTaken(unsigned char *screenshotBytes,
                           size_t screenshotSize) {
  // Do nothing for now in response to a screenshot being taken, because we
  // don't currently support the triggering of a screenshot via the WebAssembly,
  // so we know this function should never be called.
  //
  // Taking a screenshot in Doom can only be done in 'development mode', which
  // is enabled via the  "-devparm" command line argument, which we don't
  // currently allow in this implementation (in fact, we don't allow the passing
  // of ANY command line arguments to Doom).
}

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
