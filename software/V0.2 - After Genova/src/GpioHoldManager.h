#pragma once
#include <vector>

#include "driver/rtc_io.h"

class GpioHoldManager {
   public:
    void add(gpio_num_t pin) { pins.push_back(pin); }

    void enableAll() {
        gpio_deep_sleep_hold_en();
        for (auto pin : pins) {
            gpio_hold_en(pin);
        }
    }

    void disableAll() {
        gpio_deep_sleep_hold_dis();
        for (auto pin : pins) {
            gpio_hold_dis(pin);
        }
    }

   private:
    std::vector<gpio_num_t> pins;
};