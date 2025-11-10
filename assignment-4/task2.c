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
