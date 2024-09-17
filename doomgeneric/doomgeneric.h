#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdlib.h>
#include <stdint.h>

#define DOOMGENERIC_RESX 640
#define DOOMGENERIC_RESY 400

// number of allowed save game slots
#define SAVEGAMECOUNT 6

extern uint32_t *DG_ScreenBuffer;

struct DG_WadFileBytes {
  unsigned char *data;
  size_t byteLength;
};

struct DB_BytesForAllWads {
  struct DG_WadFileBytes iWad;
  struct DG_WadFileBytes *pWads;
  int numberOfPWads;
};

typedef struct save_game_reader {
  // Read bytes and return the number of bytes read
  size_t (*ReadBytes)(struct save_game_reader *reader,
                      unsigned char *destination, size_t numberOfBytes);
  // Return the number of bytes that have been read so far
  long (*BytesReadSoFar)(struct save_game_reader *reader);
  // Close the reader, and free all associated resources, including the reader
  // itself. Use of the given save_game_reader after this function call has
  // returned will result in undefined behavior.
  int (*Close)(struct save_game_reader *reader);
} save_game_reader_t;

typedef struct save_game_writer {
  // Write bytes and return the number of bytes written
  size_t (*WriteBytes)(struct save_game_writer *writer, unsigned char *source,
                       size_t numberOfBytes);
  // Return the number of bytes that have been written so far
  long (*BytesWrittenSoFar)(struct save_game_writer *writer);
  // Close the writer, committing the data written, and free all associated
  // resources, including the writer itself. Use of the given save_game_writer
  // after this function call has returned will result in undefined behavior
  int (*Close)(struct save_game_writer *writer);
} save_game_writer_t;

void doomgeneric_Create(int argc, char **argv);
void doomgeneric_Tick();

// Implement below functions for your platform
void DG_Init();
struct DB_BytesForAllWads DG_GetWads();
void DG_DrawFrame();
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs();
int DG_GetKey(int *pressed, unsigned char *key);
void DG_SetWindowTitle(const char *title);
// Return NULL if there is no save game data saved to the given slot
save_game_reader_t *DG_OpenSaveGameReader(int saveGameSlot);
save_game_writer_t *DG_OpenSaveGameWriter(int saveGameSlot);
void DG_DemoRecorded(const char *demoName, unsigned char *demoBytes,
                     size_t demoSize);
void DG_PCXScreenshotTaken(unsigned char *screenshotBytes,
                           size_t screenshotSize);

#endif // DOOM_GENERIC
