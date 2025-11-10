READ ON GITHUB: [https://github.com/balazshevesi/1dt301-lab-assignment-2/blob/main/assignment-2/ReportAssignment2.md](https://github.com/balazshevesi/1dt301-lab/blob/main/assignment-3/ReportAssignment3.md)

# Lab 3, Subroutines & Inputs (RP2040 / Raspberry Pi Pico)

## [Task 1, Average of 8 Numbers](task1.S)

### Design

- Data placed in `.data` and loaded with `ldr r0, =my_array`
- **average()** sums 8 `word`s, then divides by 8 with `lsrs r0, r2, #3` (power-of-two length, shift).
- Result printed continuously via `printf` in a loop (visible in Minicom).

### Key code (excerpt)

```asm
average:
    movs    r2, #0
1:  ldr     r3, [r0]
    adds    r0, r0, #4
    adds    r2, r2, r3
    subs    r1, r1, #1
    bne     1b
    lsrs    r0, r2, #3    @ divide by 8
    bx      lr
```

Full program in `task1.S`.

### Result

For `my_array = {10,20,30,40,50,60,70,80}`, the terminal shows repeatedly:

```
Average value 45
```

(as printed by `printf("Average value %d\n", avg)`).

## [Task 2, Buttons & LEDs via SDK (C from ASM)](task2.S)

### Design

- **Init and direction (SDK):** Each pin is initialized with `gpio_init(pin)`, then direction is set via the C wrapper `link_gpio_set_dir(pin, out)`.

  - LEDs **GP0/GP1**, `GPIO_OUT` (outputs).
  - Buttons **GP2/GP3**, `GPIO_IN` (inputs).

- **Pull-ups on inputs:** Each button is biased using `link_gpio_pull_up(pin)` so the line is stable when not pressed. (Idle = logic high with this wiring.) **Note:** the file currently enables pull-up twice on **BTN_PIN_1**; the second call should target **BTN_PIN_2**.
- **Reading inputs (SDK):** `link_gpio_get(pin)` returns the logic level of a pin; we read **GP2** into `r5` and **GP3** into `r6` once per loop.
- **Output control (SDK):** `link_gpio_put(pin, value)` drives each LED. Using single-pin writes keeps the code simple; the shim also exposes `link_gpio_put_masked` if batch updates are desired.
- **Logic / double-press ignore:**

  1. If **BTN1 (GP2) == 1**, check BTN2; if BTN2 also **1**, ignore (no state change); else **turn both LEDs ON**.
  2. Else, if **BTN2 (GP3) == 1**, **turn both LEDs OFF**.
  3. Else loop.
     This matches the “ignore when both are down” requirement while prioritizing BTN1’s ON action.

### Call flow

`link_gpio_get(GP2) → r5`, `link_gpio_get(GP3) → r6` → branch to `leds_on` / `leds_off` or fall through to `loop` (no change). LED writes use two `link_gpio_put(pin, v)` calls (GP0 then GP1).

## [Task 3, Direct Register I/O (No SDK for I/O)](task3.S)

### Design

- **Goal:** Same behavior as Task 2, but **no C calls in the I/O loop**. All reads/writes go straight to **RP2040 hardware registers** via SIO/IO_BANK0/PADS. (C can still be used for init if wanted; here, init is also done in assembly.)
- **Key base addresses & offsets (from RP2040 docs):**

  - `SIO_BASE = 0xD0000000` (fast GPIO read/write), with offsets:
    `GPIO_IN = 0x004`, `GPIO_OUT_SET = 0x014`, `GPIO_OUT_CLR = 0x018`, `GPIO_OE_SET = 0x024`, `GPIO_OE_CLR = 0x028`.
  - `IO_BANK0_BASE = 0x40014000` (pin function mux). Each pin’s CTRL word is `GPIO_CTRL_OFFS + 8*pin`; writing `FUNCSEL_SIO (5)` selects SIO for that pin.
  - `PADS_BANK0_BASE = 0x4001C000` (electrical pad config). For each pin, the pad word is `PADS_PIN_OFFS + 4*pin`. We set **PUE_BIT** and **SCHMITT_BIT**, and clear **PDE_BIT** to enable pull-up + Schmitt, no pull-down.

- **Macros (clarity & single-instruction sequences):**

  - `set_func_sio pin`, route **GPIO → SIO** by writing the pin’s IO_BANK0 CTRL word.
  - `sio_dir_out pin` / `sio_dir_in pin`, set/clear **Output Enable** in SIO for a pin.
  - `sio_put_high pin` / `sio_put_low pin` → write to **GPIO_OUT_SET/CLR** bit for that pin.
  - `pads_pullup pin`, **read-modify-write** the PADS word: `r2 = (r2 & ~PDE_BIT) | (PUE_BIT | SCHMITT_BIT)`.

- **Pin roles:** **GP0/GP1** (LEDs, outputs), **GP2/GP3** (buttons, inputs with pull-ups).

### Initialization sequence

1. `set_func_sio` for **GP0..GP3** → puts pins into SIO mode.
2. `sio_dir_out` for **GP0/GP1**; `sio_dir_in` for **GP2/GP3**.
3. `pads_pullup` for **GP2/GP3** (pull-up + Schmitt trigger).

### Main loop (register-level I/O only)

- **Read once:** `ldr r1, [SIO_BASE + GPIO_IN]` to sample **all** GPIO levels in one load.
- **Extract button bits:** shift by pin index and AND with `1` → `r5 = ((r1 >> GP2) & 1)`, `r6 = ((r1 >> GP3) & 1)`.
- **Branching:** same policy as Task 2—if BTN1 is asserted and BTN2 not, **LEDs ON**; else if BTN2 asserted, **LEDs OFF**; if both asserted, **ignore**.
- **Write outputs:** `sio_put_high/low` macros hit **GPIO_OUT_SET/CLR** for GP0 and GP1.
  This achieves the full behavior with **no SDK calls inside the loop**.

### Notes

- Doing one `GPIO_IN` read per loop avoids inconsistent reads between pins.
- Schmitt input improves noise immunity on buttons; tiny delay is acceptable.
