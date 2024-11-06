#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "doom_exports.h"
#include "doom_imports.h"
#include "doom_utils.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Usage: %s path-to-Doom-WebAssembly-module [pathToWad ...]\n",
           argv[0]);
    return 1;
  }

  const char *pathToDoomWasmModule = argv[1];

  doom_module_config_t config;
  config.numberOfWadFiles = argc - 2;
  config.pathsToWadFiles = (config.numberOfWadFiles > 0) ? argv + 2 : NULL;

  doom_module_instance_t *doom_instance = NULL;

  doom_module_error_t *error =
      doom_module_instance_new(pathToDoomWasmModule, &config, &doom_instance);

  if (!error) {
    doom_module_context_t *context =
        doom_module_instance_context(doom_instance);

    error = run_game(context);

    doom_module_instance_delete(doom_instance);
  }

  if (error) {
    fprintf(stderr, "An error occurred!\n%s\n", error->message);
    doom_module_error_delete(error);
    return 1;
  } else {
    return 0;
  }
}

// Provide implementations of all functions declared in doom_utils.h

void write_i32_to_wasm_memory(uint8_t *address, int32_t val) {
  uint32_t uVal = (uint32_t)val;
  *(address + 0) = (uVal >> (8 * 0)) & 0xff;
  *(address + 1) = (uVal >> (8 * 1)) & 0xff;
  *(address + 2) = (uVal >> (8 * 2)) & 0xff;
  *(address + 3) = (uVal >> (8 * 3)) & 0xff;
}

char *sprintf_with_malloc(const char *format, ...) {
  va_list args;
  va_start(args, format);

  char *result = vsprintf_with_malloc(format, args);

  va_end(args);

  return result;
}

char *vsprintf_with_malloc(const char *format, va_list args) {
  va_list args2;
  va_copy(args2, args);

  // +1 to account for the terminating \0 character
  size_t charactersNeeded = 1 + vsnprintf(NULL, 0, format, args);
  char *result = malloc(charactersNeeded);
  vsprintf(result, format, args2);

  va_end(args2);

  return result;
}
