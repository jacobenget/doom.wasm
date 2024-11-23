#include <stdio.h>
#include <unistd.h>

#include "doom_wasm.h"

/*
 * This file provides implementations for all (5) WASI-snapshot-preview1
 * functions needed by Doom:
 *   - proc_exit
 *   - fd_fdstat_get
 *   - fd_seek
 *   - fd_write
 *   - fd_close
 *
 * These functions are exported from the WebAssembly module to be hooked up
 * to actual calls to WASI functions through a somewhat custom external
 * mechanism.
 *
 *
 * Documentation on WASI-snapshot-preview 1:
 *    https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md
 *
 * Note: many macros and struct definitions used here (e.g. __wasi_fdstat_t) are
 * taken from wasi/api.h:
 *    https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/headers/public/wasi/api.h
 * and all those used here are, apparently, brought in by including <stdio.h>
 * when building with the wasi-sdk, which we're doing.
 */

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

EXPORT void internal__proc_exit(int32_t exitCode);
EXPORT int32_t internal__fd_fdstat_get(int32_t fd, __wasi_fdstat_t *fdstat);
EXPORT int32_t internal__fd_seek(int32_t fd, int64_t offset, int32_t whence,
                                 int32_t offset_out_ptr);
EXPORT int32_t internal__fd_write(int32_t fd, const __wasi_ciovec_t *iovs,
                                  int32_t iovs_len, uint32_t *nwritten);
EXPORT int32_t internal__fd_close(int32_t fd);

// *****************************************************************************
// *                              IMPLEMENTATIONS                              *
// *                                                                           *
// *             which may use functions imported via WebAssembly              *
// *****************************************************************************

void internal__proc_exit(int32_t exitCode) {
  // Effectively do nothing for now during "process exit".
  //
  // Why? We have yet to encounter a situation where this function is called.
  // This IS surprising, but is likely due to the game just 'crashing' whenever
  // the user tries to exit the game.
  //
  // For instance, when run in the browser, the browser console logs the
  // following error when an attempt is made to exit the game from the "Quit
  // Game" menu option:
  //    Uncaught RuntimeError: null function or function signature mismatch
  //
  // So, the plan should be to first fix the issues preventing the game from
  // cleanly exiting. Then the implementation of this function will matter, and
  // we can implement it by calling an imported onExit(exitCode) function
  // provided from the outside world, giving the outside world a chance to
  // respond to the user wishing to exit.
  fprintf(stderr, "Surprise! `internal__proc_exit` was called but we haven't "
                  "yet implemented it to respond to requests to exit!");
}

int32_t internal__fd_fdstat_get(int32_t fd, __wasi_fdstat_t *fdstat) {

  // The only file descriptors this wasi-snapshot-preview1 implementation
  // supports are STDOUT_FILENO and STDERR_FILENO. Those file descriptors are
  // character devices to which the user can write, and that's it.
  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    fdstat->fs_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
    fdstat->fs_flags = 0; // no flags
    fdstat->fs_rights_base = __WASI_RIGHTS_FD_WRITE;
    fdstat->fs_rights_inheriting = 0; // no rights are inherited

    return __WASI_ERRNO_SUCCESS; // SUCCESS
  } else {
    return __WASI_ERRNO_BADF;
  }
}

int32_t internal__fd_seek(int32_t fd, int64_t offset, int32_t whence,
                          int32_t offset_out_ptr) {
  // Seeking is not allowed on any file descriptors we support.
  return __WASI_ERRNO_NOTSUP;
}

// The imported functions `info` and `error` shall be called exactly once for
// each line of output written to stdout and stderr, respectively. To support
// that we need to buffer the output we receive until we encounter a newline
// character.
//
// We'll just leverage buffers with a fixed size for now, one for stdout and one
// for stderr. This will very likely meet our needs as it's unlikely that a
// single line written out to either stdout or stderr (the only two file
// descriptors we support) will be longer than the number of characters our
// fixed-sized buffer supports.
//
// If we're in the unlikely situation that we're about to overflow the buffer
// we'll just flush the current contents of the buffer, effectively inserting
// our own newline to enforce that no line flushed to the user is longer than
// the buffer size.
#define LINE_BUFFER_BYTE_LENGTH 1024
static char lineBuffer_info[LINE_BUFFER_BYTE_LENGTH] = {0};
static size_t lengthOfCurrentLine_info = 0;
static char lineBuffer_error[LINE_BUFFER_BYTE_LENGTH] = {0};
static size_t lengthOfCurrentLine_error = 0;

int32_t internal__fd_write(int32_t fd, const __wasi_ciovec_t *iovs,
                           int32_t iovs_len, uint32_t *nwritten) {

  void (*characterSink)(const char *message, size_t length) =
      (fd == STDOUT_FILENO) ? onInfoMessage : onErrorMessage;
  char *lineBuffer = (fd == STDOUT_FILENO) ? lineBuffer_info : lineBuffer_error;
  size_t *lengthOfCurrentLine_ptr = (fd == STDOUT_FILENO)
                                        ? &lengthOfCurrentLine_info
                                        : &lengthOfCurrentLine_error;

  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    *nwritten = 0;

    for (int i = 0; i < iovs_len; i++) {
      size_t charsConsumed = 0;
      while (charsConsumed < iovs[i].buf_len) {
        char nextCharacter = (char)iovs[i].buf[charsConsumed];
        // If the next character is a newline character then we'll flush the
        // buffer, not including the newline, to the user
        if (nextCharacter == '\n') {
          characterSink(lineBuffer, *lengthOfCurrentLine_ptr);
          *lengthOfCurrentLine_ptr = 0;
        } else {
          // Otherwise, copy the next character from the buffer to the line
          // buffer

          // If we're about to overflow the buffer then we'll flush the buffer
          // to the user
          if ((*lengthOfCurrentLine_ptr) + 1 >= LINE_BUFFER_BYTE_LENGTH) {
            characterSink(lineBuffer, *lengthOfCurrentLine_ptr);
            *lengthOfCurrentLine_ptr = 0;
          }

          lineBuffer[*lengthOfCurrentLine_ptr] = nextCharacter;
          (*lengthOfCurrentLine_ptr)++;
        }

        charsConsumed++;
      }

      *nwritten += iovs[i].buf_len;
    }

    return __WASI_ERRNO_SUCCESS;
  } else {
    return __WASI_ERRNO_BADF;
  }
}

int32_t internal__fd_close(int32_t fd) {
  // Only file descriptors this wasi-implementation supports are STDOUT_FILENO
  // and STDERR_FILENO, which we don't expect the user to ever 'close', so this
  // function should never be called, so we're just going to report that
  // closing a file descriptor is not supported.
  return __WASI_ERRNO_NOTSUP;
}
