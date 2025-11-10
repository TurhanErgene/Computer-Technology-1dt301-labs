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
