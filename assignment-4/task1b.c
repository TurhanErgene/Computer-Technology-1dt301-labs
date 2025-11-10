#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/regs/sio.h"

#define LED     0
#define BTN_ON  2
#define BTN_OFF 3

static void init_pins(void){
   gpio_init(LED);     gpio_set_dir(LED, GPIO_OUT);
   gpio_init(BTN_ON);  gpio_set_dir(BTN_ON, GPIO_IN);  gpio_pull_up(BTN_ON);
   gpio_init(BTN_OFF); gpio_set_dir(BTN_OFF, GPIO_IN); gpio_pull_up(BTN_OFF);
}

static inline void led_on(void)  { sio_hw->gpio_set = (1u << LED); }
static inline void led_off(void) { sio_hw->gpio_clr = (1u << LED); }

int main() {
   stdio_init_all();
   init_pins();

   while (1) {
       uint32_t in = sio_hw->gpio_in;          // tüm pin seviyeleri
       int on_high  = (in >> BTN_ON)  & 1u;    // pull-up: 1=basılı değil, 0=basılı
       int off_high = (in >> BTN_OFF) & 1u;

       if (on_high == 0 && off_high == 0) { tight_loop_contents(); continue; }
       if (on_high == 0)      led_on();
       else if (off_high == 0) led_off();

       sleep_ms(10);
   }
}



