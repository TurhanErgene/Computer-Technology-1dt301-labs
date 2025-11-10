The GITHUB repo: https://github.com/TurhanErgene/Computer-Technology-1dt301-labs/tree/main/assignment-4

# Lab 4, C programming with interrupts

## Task 1 – Input and Output in C

Use the same setup as in Lab 3:  
- **LED → GP0**  
- **BTN_ON → GP2**  
- **BTN_OFF → GP3**

---

## Task 1a – Basic I/O using SDK functions

### Design  
All GPIO pins are initialized with the standard SDK functions:
```c
gpio_init(pin);
gpio_set_dir(pin, GPIO_OUT or GPIO_IN);
gpio_pull_up(pin);
```

gpio_get() reads button states, and gpio_put() drives the LED.
Each button uses an internal pull-up, so an unpressed button reads 1, pressed reads 0.

### Behavior
- **If both buttons are pressed → ignore (no change).**
- **If BTN_ON pressed → turn LED ON.**
- **If BTN_OFF pressed → turn LED OFF.**
  A short delay (sleep_ms(10)) limits loop frequency.

### Result
LED correctly turns on and off depending on which button is pressed.
Operation verified on serial output and board.


## Task 1b – Direct register access via SIO
### Design  
This version replaces SDK GPIO control with direct access to the SIO registers, using the structure sio_hw.
Initialization still uses SDK functions for simplicity.
- **Read inputs: sio_hw->gpio_in**
- **Set LED: sio_hw->gpio_set = (1u << LED)**
- **Clear LED: sio_hw->gpio_clr = (1u << LED)**
This gives lower latency and avoids function-call overhead.

### Result
LED responds correctly to button inputs using SDK-level I/O functions.

## Task 1c – Controlling Two LEDs Simultaneously

### Design  
This task extends Task 1b by adding a second LED on **GP1**, so both LEDs turn on or off together.  
Both are controlled directly via the **SIO registers**, bypassing SDK-level GPIO functions for speed.  
A bitmask is defined to control multiple GPIO pins simultaneously.

```c
#define LED0    0
#define LED1    1
#define BTN_ON  2
#define BTN_OFF 3
#define LEDS_MASK  ((1u<<LED0) | (1u<<LED1))
```
The helper functions use SIO registers:
-**leds_on() → sets both LED pins high**
-**leds_off() → clears both LED pins**

```c
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/regs/sio.h"

#define LED0    0
#define LED1    1
#define BTN_ON  2
#define BTN_OFF 3
#define LEDS_MASK  ((1u<<LED0) | (1u<<LED1))

static void init_pins(void){
   gpio_init(LED0);   gpio_set_dir(LED0, GPIO_OUT);
   gpio_init(LED1);   gpio_set_dir(LED1, GPIO_OUT);
   gpio_init(BTN_ON); gpio_set_dir(BTN_ON, GPIO_IN);  gpio_pull_up(BTN_ON);
   gpio_init(BTN_OFF);gpio_set_dir(BTN_OFF, GPIO_IN); gpio_pull_up(BTN_OFF);
}

static inline void leds_on(void)  { sio_hw->gpio_set = LEDS_MASK; }
static inline void leds_off(void) { sio_hw->gpio_clr = LEDS_MASK; }

int main() {
   stdio_init_all();
   init_pins();

   while (1) {
       uint32_t in = sio_hw->gpio_in;
       int on_high  = (in >> BTN_ON)  & 1u;   // 0 = pressed
       int off_high = (in >> BTN_OFF) & 1u;

       if (on_high == 0 && off_high == 0) { tight_loop_contents(); continue; }
       if (on_high == 0)      leds_on();
       else if (off_high == 0) leds_off();

       sleep_ms(10);
   }
}

```
-**BTN_ON (GP2) pressed → both LEDs turn ON.**
-**BTN_OFF (GP3) pressed → both LEDs turn OFF.**
-**Both pressed together → ignored (no state change).**
-**The loop includes a short delay (sleep_ms(10)) for stable reading.**

### Result
Both LEDs respond simultaneously to button inputs.
This confirms correct use of bitmask-based control and direct register I/O for multiple outputs.


## Task 2 – Binary counter with GPIO interrupts

### Design
Four LEDs on **GP0..GP3** display a 4-bit counter. Two buttons control increment and decrement:
- **BTN_INC → GP5** with pull-down, triggers on rising edge.
- **BTN_DEC → GP6** with pull-down, triggers on rising edge.

Interrupt service routine debounces per GPIO using a timestamp array and updates the counter within bounds **0..15**. Display uses `gpio_put_masked` with a 4-bit mask.

### Behavior
- Press **BTN_INC**: counter++ if `< 15`.
- Press **BTN_DEC**: counter-- if `> 0`.
- LEDs always reflect the current 4-bit value.

### Code
```c
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_BASE     0
#define LED_MASK     (0xFu << LED_BASE)
#define BTN_INC      5
#define BTN_DEC      6
#define DEBOUNCE_MS  120

static volatile uint8_t counter = 0;
static uint32_t last_ms_per_gpio[32];

static inline void show_counter(void) {
    gpio_put_masked(LED_MASK, ((uint32_t)(counter & 0xF) << LED_BASE));
}

static void button_isr(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_ms_per_gpio[gpio] < DEBOUNCE_MS) return;
    last_ms_per_gpio[gpio] = now;

    if (gpio == BTN_INC) { if (counter < 15) counter++; }
    else if (gpio == BTN_DEC) { if (counter > 0) counter--; }
    show_counter();
}

int main() {
    stdio_init_all();

    for (int p = LED_BASE; p < LED_BASE + 4; ++p) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        gpio_put(p, 0);
    }
    show_counter();

    gpio_init(BTN_INC); gpio_set_dir(BTN_INC, GPIO_IN); gpio_pull_down(BTN_INC);
    gpio_init(BTN_DEC); gpio_set_dir(BTN_DEC, GPIO_IN); gpio_pull_down(BTN_DEC);

    gpio_set_irq_enabled_with_callback(BTN_INC, GPIO_IRQ_EDGE_RISE, true, button_isr);
    gpio_set_irq_enabled(BTN_DEC, GPIO_IRQ_EDGE_RISE, true);

    while (true) tight_loop_contents();
}
```
### Result
- Power on shows 0000 on LEDs GP0..GP3.
- Press BTN_INC repeatedly: LEDs count up to 1111. Pressing again at 1111 leaves value unchanged.
- Press BTN_DEC repeatedly: LEDs count down to 0000. Pressing again at 0000 leaves value unchanged.
- Single presses generate single steps with DEBOUNCE_MS = 120. Fast chattering does not cause multi-steps.
- Verified that ISR runs only on rising edges by observing stable counts when holding the buttons.


## Task 3 – Binary counter with timer interrupt and reset button

### Design
Same LED layout on GP0..GP3. A repeating timer ticks every 1000 ms and increments the counter until it reaches 15. A reset button on GP6 uses a GPIO interrupt with rising-edge detection and per-GPIO debounce.

### Behavior
- Timer: increment counter once per second while < 15.
- BTN_RST → GP6 pressed: counter resets to 0 and display updates immediately.
- Counting resumes after reset. Stops automatically at 15.

### Code
```c
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#define LED_BASE     0
#define LED_MASK     (0xFu << LED_BASE)
#define BTN_RST      6
#define DEBOUNCE_MS  120

static volatile uint8_t counter = 0;
static uint32_t last_ms_per_gpio[32];
static repeating_timer_t tick;

static inline void show_counter(void) {
    gpio_put_masked(LED_MASK, ((uint32_t)(counter & 0xF) << LED_BASE));
}

static bool tick_cb(repeating_timer_t *t) {
    if (counter < 15) {
        counter++;
        show_counter();
    }
    return true;
}

static void gpio_isr(uint gpio, uint32_t events) {
    if (gpio != BTN_RST || !(events & GPIO_IRQ_EDGE_RISE)) return;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_ms_per_gpio[gpio] < DEBOUNCE_MS) return;
    last_ms_per_gpio[gpio] = now;

    counter = 0;
    show_counter();
}

int main() {
    stdio_init_all();

    for (int p = LED_BASE; p < LED_BASE + 4; ++p) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        gpio_put(p, 0);
    }
    show_counter();

    gpio_init(BTN_RST);
    gpio_set_dir(BTN_RST, GPIO_IN);
    gpio_pull_down(BTN_RST);
    gpio_set_irq_enabled_with_callback(BTN_RST, GPIO_IRQ_EDGE_RISE, true, gpio_isr);

    add_repeating_timer_ms(1000, tick_cb, NULL, &tick);

    while (true) tight_loop_contents();
}
```
### Result
- After reset or power on, LEDs show 0000. Counter increases to 1111 in 15 seconds, then holds.
- Pressing BTN_RST sets LEDs to 0000 immediately regardless of current value. Counting resumes with the next 1 s tick.
- Measured tick cadence is approximately 1 s. Minor jitter is within SDK timer tolerance and does not skip values.
- Debounce prevents spurious resets from mechanical bounce.
