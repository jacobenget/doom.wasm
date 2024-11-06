#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(*array))

// Write a 32-bit integer to a location in memory owned by a WebAssembly module.
//
// This function exists because numerical values in WebAssembly are always laid
// out in memory in little-endian order, no matter what the architecture the
// WebAssembly interpreter is running on.
//
// So, writing a 32-bit integer correctly to memory owned by a WebAssembly
// module requires that the 32-bit integer be written in little-endian order,
// and this function does that.
void write_i32_to_wasm_memory(uint8_t *address, int32_t val);

// `sprintf_with_malloc` accomplishes the same goal as `sprintf`:
// https://cplusplus.com/reference/cstdio/sprintf/ except that the buffer
// written to is dynamically allocated by this function.
//
// The benefit of this approach, where the caller doesn't have to provide the
// buffer, is that the caller does not have to calculate the appropriate size of
// buffer needed. Instead this function handles that calculation.
//
// The returned `char *` is owned by the caller and should be free'd
// appropriately when no longer needed.
char *sprintf_with_malloc(const char *format, ...);

// `vsprintf_with_malloc` accomplishes the same goal as `vsprintf`:
// https://cplusplus.com/reference/cstdio/sprintf/ except that the buffer
// written to is dynamically allocated by this function.
//
// The benefit of this approach, where the caller doesn't have to provide the
// buffer, is that the caller does not have to calculate the appropriate size of
// buffer needed. Instead this function handles that calculation.
//
// The returned `char *` is owned by the caller and should be free'd
// appropriately when no longer needed.
char *vsprintf_with_malloc(const char *format, va_list args);

#endif
