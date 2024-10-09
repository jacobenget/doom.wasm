#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "doomgeneric.h"
#include "doom_wasm.h"
#include "file_embedded_in_code/DOOM1.WAD.h"

/*
 * This file reconciles the WebAssembly interface (i.e. imports/exports)
 * specified in `doom_wasm.h` with Doom's interface specified in
 * `doomgeneric.h`.
 *
 * In particular this file is responsible for two things:
 *  1) Implementing all functions exported via WebAssembly,
 *      via the features provided by `doomgeneric.h`.
 *  2) Implementing all functions needed by `doomgeneric.h`,
 *      via the functions imported via WebAssembly.
 */

// *****************************************************************************
// *                                    (1)                                    *
// *                                                                           *
// *             Implement all functions exported via WebAssembly              *
// *               via the features provided by `doomgeneric.h`                *
// *                                                                           *
// *****************************************************************************

// A global cache of which keys are currently pressed down, according to what
// the user of this module has reported to via `reportKeyDown` and
// `reportKeyUp`.
static bool isKeyPressed[UINT8_MAX + 1] = {false};

void _initializeDoom() {
  // Provide 0 command line arguments to Doom.
  // Any configuration knobs we end up exposing via WebAssembly that would
  // usually be handled by command line arguments will instead be handled in a
  // different way.
  int argc = 0;
  char *argv[] = {};

  doomgeneric_Create(argc, argv);
}

void tickGame() { doomgeneric_Tick(); }

void reportKeyDown(int32_t doomKey) {
  if (0 <= doomKey && doomKey <= UINT8_MAX) {
    isKeyPressed[doomKey] = true;
  } else {
    fprintf(stderr,
            "The invalid value of %i was provided to `reportKeyDown`, which "
            "only accepts values in the range [0, %i]\n",
            doomKey, UINT8_MAX);
  }
}

void reportKeyUp(int32_t doomKey) {
  if (0 <= doomKey && doomKey <= UINT8_MAX) {
    isKeyPressed[doomKey] = false;
  } else {
    fprintf(stderr,
            "The invalid value of %i was provided to `reportKeyUp`, which only "
            "accepts values in the range [0, %i]\n",
            doomKey, UINT8_MAX);
  }
}

// *****************************************************************************
// *                                    (2)                                    *
// *                                                                           *
// *            Implement all functions needed by `doomgeneric.h`              *
// *                via the functions imported via WebAssembly                 *
// *                                                                           *
// *****************************************************************************

void DG_Init() { onGameInit(DOOMGENERIC_RESX, DOOMGENERIC_RESY); }

struct DB_BytesForAllWads DG_GetWads() {
  struct DB_BytesForAllWads result = {0};

  int numberOfWads = 0;
  size_t numberOfTotalBytesInAllWads = 0;
  wadSizes(&numberOfWads, &numberOfTotalBytesInAllWads);

  if (numberOfWads > 0) {
    unsigned char *wadData = malloc(numberOfTotalBytesInAllWads);
    int byteLengthOfEachWad[numberOfWads];
    memset(byteLengthOfEachWad, 0, sizeof(byteLengthOfEachWad));
    readWads(wadData, byteLengthOfEachWad);

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
    printf("Defaulting to loading Doom shareware WAD because no WAD data was "
           "provided\n");
    result.iWad.data = malloc(DOOM1_WAD_length);
    memcpy(result.iWad.data, DOOM1_WAD_data, DOOM1_WAD_length);
    result.iWad.byteLength = DOOM1_WAD_length;
    result.numberOfPWads = 0;
    result.pWads = 0;
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

void DG_SetWindowTitle(const char *title) {
  // Just do nothing.
  //
  // The title provided doesn't contain much useful information other than some
  // identifier for which IWAD is being used, and since the user of this
  // WebAssembly module is providing the IWAD used they already have the
  // opportunity, without the help of Doom, to do anything custom related to the
  // IWAD being used.
}

void DG_SleepMs(uint32_t ms) {
  // Doom only ever calls DG_SleepMs with an arg of '1', and in those cases Doom
  // is in a busy-wait loop polling DG_GetTicksMs between calls to DG_SleepMs(1)
  // in order to spin until a target amount of time has passed.
  //
  // In other words, Doom doesn't actually depend upon DG_SleepMs(1) doing
  // anything other than returning in a relatively short amount of time.
  //
  // We can accomplish that by just returning immediately instead of depending
  // upon some lower-level feature that accomplishes something like actual
  // sleeping.
  if (ms == 1) {
    // Do nothing. See comment above.
  } else {
    fprintf(stderr,
            "DG_SleepMs called with an `ms` value of %u, which is not 1. This "
            "was unexpected and not currently supported!",
            ms);
  }
}

uint64_t DG_GetTicksMs() { return timeInMilliseconds(); }

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
