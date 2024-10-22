#ifndef USER_INPUT_H_
#define USER_INPUT_H_

#include <stdbool.h>

enum InputEventType {
  KEY_STATE_CHANGE,
  EXIT,
};

enum KeyState {
  KEY_IS_UP,
  KEY_IS_DOWN,
};

typedef struct key_state_change {
  int32_t doomKey;
  enum KeyState newState;
} key_state_change_t;

typedef struct input_event {
  enum InputEventType eventType;
  // keyStateChange is only valid if eventType is KEY_STATE_CHANGE
  key_state_change_t keyStateChange;
} input_event_t;

typedef struct state_for_reading_global_constants
    state_for_reading_global_constants_t;

typedef int32_t
read_global_constant_i32(state_for_reading_global_constants_t *state,
                         const char *nameOfConstant);

bool pollUserInput(input_event_t *eventOut,
                   state_for_reading_global_constants_t *state,
                   read_global_constant_i32 *read_global_const);

#endif
