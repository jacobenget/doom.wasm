#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdlib.h>
#include <stdint.h>

#define DOOMGENERIC_RESX 640
#define DOOMGENERIC_RESY 400

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

#endif // DOOM_GENERIC
