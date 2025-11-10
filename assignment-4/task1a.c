#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED     0
#define BTN_ON  2
#define BTN_OFF 3

static void init_pins(void){
   gpio_init(LED);     gpio_set_dir(LED, GPIO_OUT);
   gpio_init(BTN_ON);  gpio_set_dir(BTN_ON, GPIO_IN);  gpio_pull_up(BTN_ON);
   gpio_init(BTN_OFF); gpio_set_dir(BTN_OFF, GPIO_IN); gpio_pull_up(BTN_OFF);
}

int main() {
   stdio_init_all();
   init_pins();

   while (1) {
       int on_high  = gpio_get(BTN_ON);    // 1 = not pressed, 0 = pressed
       int off_high = gpio_get(BTN_OFF);

       if (on_high == 0 && off_high == 0) { tight_loop_contents(); continue; }
       if (on_high == 0)        gpio_put(LED, 1);
       else if (off_high == 0)  gpio_put(LED, 0);

       sleep_ms(10);
   }
}



