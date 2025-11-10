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
       int on_high  = (in >> BTN_ON)  & 1u;   // 0 = basılı
       int off_high = (in >> BTN_OFF) & 1u;

       if (on_high == 0 && off_high == 0) { tight_loop_contents(); continue; }
       if (on_high == 0)      leds_on();
       else if (off_high == 0) leds_off();

       sleep_ms(10);
   }
}



