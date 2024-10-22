#include <SDL.h>

#include "doom_imports.h"

// State shared between functions
static SDL_Window *window;
static SDL_Texture *texture;

// IMPORTS

void console_onErrorMessage(int32_t messagePtr, int32_t length,
                            uint8_t *memory) {
  fprintf(stderr, "%.*s\n", length, memory + messagePtr);
}

void console_onInfoMessage(int32_t messagePtr, int32_t length,
                           uint8_t *memory) {
  fprintf(stdout, "%.*s\n", length, memory + messagePtr);
}

int32_t gameSaving_readSaveGame(int32_t gameSaveId, int32_t dataDestinationPtr,
                                uint8_t *memory) {
  return 0;
}

int32_t gameSaving_sizeOfSaveGame(int32_t gameSaveId) { return 0; }

int32_t gameSaving_writeSaveGame(int32_t gameSaveId, int32_t dataPtr,
                                 int32_t length, uint8_t *memory) {
  return 0;
}

void loading_onGameInit(int32_t width, int32_t height) {
  // Setup window and renderer
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_SHOWN, &window,
                              &renderer);
  SDL_SetWindowTitle(window, "DOOM");

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_TARGET, width, height);
}

void loading_readWads(int32_t wadDataDestinationPtr,
                      int32_t byteLengthOfEachWadPtr, uint8_t *memory) {}

void loading_wadSizes(int32_t numberOfWadsPtr,
                      int32_t numberOfTotalBytesInAllWadsPtr) {}

int64_t runtimeControl_timeInMilliseconds() { return SDL_GetTicks64(); }

void ui_drawFrame(int32_t screenBufferPtr, uint8_t *memory) {
  int textureWidth;
  SDL_QueryTexture(texture, NULL, NULL, &textureWidth, NULL);
  SDL_UpdateTexture(texture, NULL, memory + screenBufferPtr,
                    textureWidth * sizeof(uint32_t));

  SDL_Renderer *renderer = SDL_GetRenderer(window);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}
