// link_shim.c
#include "pico/stdlib.h"
#include "hardware/gpio.h"

void link_gpio_set_dir(uint pin, bool out) { gpio_set_dir(pin, out); }
void link_gpio_put(uint pin, bool v) { gpio_put(pin, v); }
void link_gpio_put_masked(uint32_t mask, uint32_t value) { gpio_put_masked(mask, value); }
void link_gpio_put_all(uint32_t v) { gpio_put_all(v); }   // <— new
uint32_t link_gpio_get(uint32_t pin) {return gpio_get(pin);}
void link_gpio_pull_up(uint pin) { gpio_pull_up(pin); }
void link_gpio_pull_down(uint pin) { gpio_pull_down(pin); }
void link_gpio_set_pulls(uint pin, bool up, bool down) { gpio_set_pulls(pin, up, down); }
