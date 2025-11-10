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
