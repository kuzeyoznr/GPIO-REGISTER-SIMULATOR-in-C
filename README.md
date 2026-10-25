# GPIO-REGISTER-SIMULATOR-in-C
A small, self-contained C project that simulates a 32-pin GPIO peripheral using a structure and bitwise operations. An active-high button on pin 3 controls an LED on output pin 5.  No microcontroller, wiring, or external libraries are required. This project is designed for learning C, register manipulation, input/output modes, and basic testing.
Quick start

Use a C11 compiler such as GCC or Clang. From the project directory:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror GPIO.c -o gpio_sim
./gpio_sim
```

On Windows with GCC installed:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror GPIO.c -o gpio_sim.exe
.\gpio_sim.exe
```

The program runs its tests first, then a fixed sequence of button presses and releases. It exits with status `0` on success and `1` on failure. It does not wait for keyboard input.

## Expected output

```text
All GPIO tests passed.

Button on input pin 3 controls LED on output pin 5:
Button RELEASED | LED OFF | IDR=0x00000000 | ODR=0x00000000
Button PRESSED  | LED ON  | IDR=0x00000008 | ODR=0x00000020
Button RELEASED | LED OFF | IDR=0x00000000 | ODR=0x00000000
Button PRESSED  | LED ON  | IDR=0x00000008 | ODR=0x00000020
Button RELEASED | LED OFF | IDR=0x00000000 | ODR=0x00000000
```

## Register model

The original `struct GPIO` and its register names are retained:

| Register | Purpose |
| --- | --- |
| `Moder` | One mode bit per pin: `0` for input, `1` for output |
| `IDR` | Simulated external input levels |
| `ODR` | Stored output levels |

Pins are numbered **0 through 31**. Initializing with `struct GPIO gpio = {0};` makes every pin an input with both level registers cleared.

This is a simplified educational model, not a hardware-compatible register layout. Actual microcontrollers can use multiple mode bits per pin and different register offsets. The simulator uses ordinary memory; it does not access hardware or model timing, pull resistors, electrical behavior, interrupts, or button debouncing.

Changing a pin's mode preserves the stored `IDR` and `ODR` bits. `gpio_read` selects the register appropriate for the current mode. Reading an output reports its stored output level, not an independently measured electrical level.

## Functions

| Function | Behavior |
| --- | --- |
| `gpio_set_mode(gpio, pin, mode)` | Select `GPIO_INPUT` or `GPIO_OUTPUT` |
| `gpio_set(gpio, pin)` | Set an output pin high |
| `gpio_clear(gpio, pin)` | Set an output pin low |
| `gpio_read(gpio, pin)` | Read `IDR` for inputs or `ODR` for outputs |
| `gpio_simulate_input(gpio, pin, level)` | Apply a simulated `0` or `1` to an input |
| `update_led(gpio)` | Copy the button state on pin 3 to LED pin 5 |

Mutating functions return `1` for success or `0` for invalid usage. `gpio_read` returns `0` or `1` for a valid pin and `-1` for invalid usage. Check that result explicitly rather than treating every nonzero value as a high level.

Null pointers and pins outside 0–31 are rejected before any register access or shift. Output operations reject input pins, simulated input operations reject output pins, and invalid modes or levels are rejected. Rejected calls leave the registers unchanged. As in ordinary C APIs, a non-null pointer must point to a valid `struct GPIO` object.

## How the bit operations work

A mask selects one pin:

```c
uint32_t mask = UINT32_C(1) << pin;
```

- `gpio->ODR |= mask` sets the selected bit while preserving other bits.
- `gpio->ODR &= ~mask` clears the selected bit while preserving other bits.
- `(value & mask) != 0U` checks whether the selected bit is set.

Pin 3 corresponds to hexadecimal `0x00000008`; pin 5 corresponds to `0x00000020`. The fixed-width constant and `PRIX32` formatting keep the code appropriate for `uint32_t`.

The demonstration configures pin 3 as an input and pin 5 as an output. Each simulated button level updates `IDR`; `update_led` reads that level and sets or clears the LED bit in `ODR`. Edit `button_levels` in `run_demo` to change the sequence.

## Built-in tests

`run_tests` checks:

- Initial state and input/output mode restrictions.
- Setting, clearing, repeated operations, and preservation of other output bits.
- Boundary pins 0 and 31.
- Button press/release behavior and LED state.
- Register selection when changing modes.
- Invalid pins, null pointers, invalid mode values, and invalid input levels.
- Rejection of incorrect button/LED configurations.

A failed check prints its line number and condition and stops the program with a nonzero exit status. Tests stay enabled even when `NDEBUG` is defined.

## Changes from the original project

- Completed the unfinished `main` and added a repeatable demonstration.
- Preserved the structure and reusable set/clear/read approach.
- Added mode configuration and separate simulated input handling.
- Made reads select `IDR` or `ODR` according to the pin's mode.
- Standardized error return values and kept printing in the tests/demo.
- Added boundary/error checks and portable 32-bit hexadecimal output.

## Files

```text
GPIO.c       Complete implementation, tests, and demonstration
README.md    Build instructions and project explanation
.gitignore   Excludes generated binaries and temporary files
```

The project builds as one source file so it is easy to inspect and modify. A useful next step is splitting the reusable GPIO functions into `gpio.h` and `gpio.c`, with separate demonstration and test files.
