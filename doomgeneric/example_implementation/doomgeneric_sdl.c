// doomgeneric for cross-platform development library 'Simple DirectMedia Layer'

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <SDL.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned int key) {
  switch (key) {
  case SDLK_RETURN:
    key = KEY_ENTER;
    break;
  case SDLK_ESCAPE:
    key = KEY_ESCAPE;
    break;
  case SDLK_LEFT:
    key = KEY_LEFTARROW;
    break;
  case SDLK_RIGHT:
    key = KEY_RIGHTARROW;
    break;
  case SDLK_UP:
    key = KEY_UPARROW;
    break;
  case SDLK_DOWN:
    key = KEY_DOWNARROW;
    break;
  case SDLK_LCTRL:
  case SDLK_RCTRL:
    key = KEY_FIRE;
    break;
  case SDLK_SPACE:
    key = KEY_USE;
    break;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:
    key = KEY_RSHIFT;
    break;
  case SDLK_LALT:
  case SDLK_RALT:
    key = KEY_LALT;
    break;
  case SDLK_F2:
    key = KEY_F2;
    break;
  case SDLK_F3:
    key = KEY_F3;
    break;
  case SDLK_F4:
    key = KEY_F4;
    break;
  case SDLK_F5:
    key = KEY_F5;
    break;
  case SDLK_F6:
    key = KEY_F6;
    break;
  case SDLK_F7:
    key = KEY_F7;
    break;
  case SDLK_F8:
    key = KEY_F8;
    break;
  case SDLK_F9:
    key = KEY_F9;
    break;
  case SDLK_F10:
    key = KEY_F10;
    break;
  case SDLK_F11:
    key = KEY_F11;
    break;
  case SDLK_EQUALS:
  case SDLK_PLUS:
    key = KEY_EQUALS;
    break;
  case SDLK_MINUS:
    key = KEY_MINUS;
    break;
  default:
    key = tolower(key);
    break;
  }

  return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode) {
  unsigned char key = convertToDoomKey(keyCode);

  unsigned short keyData = (pressed << 8) | key;

  s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
  s_KeyQueueWriteIndex++;
  s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}
static void handleKeyInput() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      puts("Quit requested");
      atexit(SDL_Quit);
      exit(1);
    }
    if (e.type == SDL_KEYDOWN) {
      // KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
      // printf("KeyPress:%d sym:%d\n", e.xkey.keycode, sym);
      addKeyToQueue(1, e.key.keysym.sym);
    } else if (e.type == SDL_KEYUP) {
      // KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
      // printf("KeyRelease:%d sym:%d\n", e.xkey.keycode, sym);
      addKeyToQueue(0, e.key.keysym.sym);
    }
  }
}

void DG_Init() {
  window =
      SDL_CreateWindow("DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       DOOMGENERIC_RESX, DOOMGENERIC_RESY, SDL_WINDOW_SHOWN);

  // Setup renderer
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  // Clear winow
  SDL_RenderClear(renderer);
  // Render the rect to the screen
  SDL_RenderPresent(renderer);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_TARGET, DOOMGENERIC_RESX,
                              DOOMGENERIC_RESY);
}

void DG_DrawFrame() {
  SDL_UpdateTexture(texture, NULL, DG_ScreenBuffer,
                    DOOMGENERIC_RESX * sizeof(uint32_t));

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);

  handleKeyInput();
}

void DG_SleepMs(uint32_t ms) { SDL_Delay(ms); }

uint32_t DG_GetTicksMs() { return SDL_GetTicks(); }

int DG_GetKey(int *pressed, unsigned char *doomKey) {
  if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
    // key queue is empty
    return 0;
  } else {
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex++;
    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

    *pressed = keyData >> 8;
    *doomKey = keyData & 0xFF;

    return 1;
  }

  return 0;
}

//
// Determine the length of an open file.
//
static long M_FileLength(FILE *handle) {
  // save the current position in the file
  long savedpos = ftell(handle);

  // jump to the end and find the length
  fseek(handle, 0, SEEK_END);
  long length = ftell(handle);

  // go back to the old location
  fseek(handle, savedpos, SEEK_SET);

  return length;
}

// These values are initialized upon startup
char *pathToIWad;
char **pathsToPWads;
int numberOfPathsToPWads;

//
// Read in an entire WAD file and return its length and bytes in a struct.
// The caller takes ownership of the memory allocated for these bytes.
//
static struct DG_WadFileBytes readWadFile(const char *pathToWadFile) {
  struct DG_WadFileBytes result = {0};

  FILE *handle = fopen(pathToWadFile, "rb");
  if (handle != NULL) {
    long fileLength = M_FileLength(handle);
    unsigned char *wadData = malloc(fileLength);
    if (wadData != NULL) {
      size_t count = fread(wadData, 1, fileLength, handle);
      fclose(handle);

      // If the read of the entire file was successful
      if (count == fileLength) {
        result.data = wadData;
        result.byteLength = fileLength;
      } else {
        fprintf(stderr, "There was an issue reading WAD data from file '%s'\n",
                pathToWadFile);
      }
    } else {
      fprintf(stderr,
              "Failed to allocate %ld bytes of memory for WAD data when "
              "reading file '%s'\n",
              fileLength, pathToWadFile);
    }
  } else {
    fprintf(stderr,
            "Failed to open file when attempting to load WAD data: '%s'\n",
            pathToWadFile);
  }

  return result;
}

struct DB_BytesForAllWads DG_GetWads() {
  struct DB_BytesForAllWads result = {0};

  result.iWad = readWadFile(pathToIWad);

  result.pWads = malloc(sizeof(struct DG_WadFileBytes) * numberOfPathsToPWads);
  if (result.pWads != NULL) {
    result.numberOfPWads = numberOfPathsToPWads;

    for (int i = 0; i < numberOfPathsToPWads; i++) {
      result.pWads[i] = readWadFile(pathsToPWads[i]);
    }
  } else {
    fprintf(stderr, "Failed to allocate memory for %d PWADs\n",
            numberOfPathsToPWads);
  }

  return result;
}

void DG_SetWindowTitle(const char *title) {
  if (window != NULL) {
    SDL_SetWindowTitle(window, title);
  }
}

static int findIndexOfString(char *needle, char **haystack, size_t haystackSize,
                             int startIndex) {
  for (int i = startIndex; i < haystackSize; i++) {
    if (!strcasecmp(needle, haystack[i])) {
      return i;
    }
  }

  return -1;
}

static int findIndexOfInt(int needle, int *haystack, size_t haystackSize,
                          int startIndex) {
  for (int i = startIndex; i < haystackSize; i++) {
    if (needle == haystack[i]) {
      return i;
    }
  }

  return -1;
}

int main(int argc, char **argv) {

  int indexOfIWadArg = findIndexOfString("-iwad", argv, argc, 1);
  int indexOfFileArg = findIndexOfString("-file", argv, argc, 1);

  int indexOfIWadPath = (indexOfIWadArg == -1) ? -1 : indexOfIWadArg + 1;
  if (indexOfIWadPath < 0 || indexOfIWadPath >= argc) {
    printf("Usage: %s -iwad <path to IWAD> [-file [<path to PWAD> ...]] [any "
           "other args supported by Doom]]\n",
           argv[0]);
    return 1;
  } else {
    int numberOfArgsProcessed = 0;
    int indicesOfArgsProcessed[argc];

    ////////////////////////////////////////////////////////
    // Do some custom processing of command line arguments
    ////////////////////////////////////////////////////////

    // Set IWAD details, provided via command line arguments
    {
      pathToIWad = argv[indexOfIWadPath];
      // We've processed both `-iwad` arg and its related path
      indicesOfArgsProcessed[numberOfArgsProcessed++] = indexOfIWadArg;
      indicesOfArgsProcessed[numberOfArgsProcessed++] = indexOfIWadPath;

      printf("Game data: Using this IWAD: %s\n", pathToIWad);
    }

    // Set PWADs details, provided via command line arguments
    if (indexOfFileArg != -1) {
      // We've processed the `-file` arg
      indicesOfArgsProcessed[numberOfArgsProcessed++] = indexOfFileArg;

      int indexOfPotentialPWadPath = indexOfFileArg + 1;
      while (indexOfPotentialPWadPath < argc &&
             argv[indexOfPotentialPWadPath][0] != '-') {
        // We've processed this PWAD path
        indicesOfArgsProcessed[numberOfArgsProcessed++] =
            indexOfPotentialPWadPath;
        numberOfPathsToPWads++;
        indexOfPotentialPWadPath++;
      }
      // If we found at least one PWAD path, we can set `pathsToPWads` to point
      // to the very first one found
      if (numberOfPathsToPWads) {
        int indexOfFirstPWadPath = indexOfFileArg + 1;
        pathsToPWads = argv + indexOfFirstPWadPath;

        printf("Game data: Using these %d PWAD files, in order:\n",
               numberOfPathsToPWads);
        for (int i = 0; i < numberOfPathsToPWads; i++) {
          printf("    %s\n", pathsToPWads[i]);
        }
      }
    }

    // Create a new argv and argc to send to Doom, which should contain exactly
    // the command line arguments that were not processed
    int argcAfterProcessing = argc - numberOfArgsProcessed;
    char *argvAfterProcessing[argcAfterProcessing];
    // Copy over the arguments that were not processed, so they are passed as-is
    // to Doom
    for (int src = 0, dest = 0; src < argc; src++) {
      bool argsWasNotProcessed = findIndexOfInt(src, indicesOfArgsProcessed,
                                                numberOfArgsProcessed, 0) == -1;
      if (argsWasNotProcessed) {
        argvAfterProcessing[dest] = argv[src];
        dest++;
      }
    }

    printf("Calling doomgeneric_Create with these args: ");
    for (int i = 0; i < argcAfterProcessing; i++) {
      printf("%s ", argvAfterProcessing[i]);
    }
    printf("\n");

    doomgeneric_Create(argcAfterProcessing, argvAfterProcessing);

    for (int i = 0;; i++) {
      doomgeneric_Tick();
    }

    return 0;
  }
}
