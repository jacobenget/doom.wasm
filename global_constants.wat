(module
  ;;
  ;; Some keycode values supported by Doom
  ;;
  ;; Many 8-bit keycodes that Doom responds to (e.g. the codes for the letters
  ;; 'a' through 'z') are just the ascii code of a printable character that
  ;; directly corresponds to the keycode, and in that case it's straightforward
  ;; to send the appropriate keycode to Doom exactly when the related key is
  ;; pressed/unpressed.
  ;;
  ;; Other Doom keycodes are either ASCII values that aren't printable (e.g.
  ;; 0x8, the code for 'backspace' in ASCII) or have no ASCII equivalent (e.g.
  ;; the code for KEY_USE). In those cases it's not so clear from the outside
  ;; what value to send to Doom to relay that one of those keys has been pressed.
  ;;
  ;; Here we associate those magic keycodes (i.e. those that are not a printable
  ;; ASCII value) with names, as global constants, to give the user a way to
  ;; reference these keycodes by name.
  ;;
  ;; Note: here's documentation on the controls of vanilla Doom:
  ;;  https://doom.fandom.com/wiki/Controls
  ;; This documentation provides direction on how one might map a physical key
  ;; to these magic keycodes, helpful in the cases where the name of the keycode
  ;; doesn't conceptually map to any given key (e.g. KEY_FIRE, KEY_STRAFE_L, ...).
  ;;
  (global $0 (export "KEY_LEFTARROW") i32 (i32.const 0xac))
  (global $1 (export "KEY_RIGHTARROW") i32 (i32.const 0xae))
  (global $2 (export "KEY_UPARROW") i32 (i32.const 0xad))
  (global $3 (export "KEY_DOWNARROW") i32 (i32.const 0xaf))
  (global $4 (export "KEY_STRAFE_L") i32 (i32.const 0xa0))
  (global $5 (export "KEY_STRAFE_R") i32 (i32.const 0xa1))
  (global $6 (export "KEY_FIRE") i32 (i32.const 0xa3))
  (global $7 (export "KEY_USE") i32 (i32.const 0xa2))
  (global $8 (export "KEY_SHIFT") i32 (i32.const 0xb6))
  (global $9 (export "KEY_TAB") i32 (i32.const 0x09))
  (global $10 (export "KEY_ESCAPE") i32 (i32.const 0x1b))
  (global $11 (export "KEY_ENTER") i32 (i32.const 0x0d))
  (global $12 (export "KEY_BACKSPACE") i32 (i32.const 0x7f))
  (global $13 (export "KEY_ALT") i32 (i32.const 0xb8))
)
