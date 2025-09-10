#include "DeepSleepHandler.h"

#include "Arduino.h"
#include "driver/rtc_io.h"
#include "esp_task_wdt.h"
#include "soc/rtc_io_reg.h"

constexpr uint64_t BUTTON_PIN_BITMASK(int gpio) { return 1ULL << gpio; }
DeepSleepHandler::DeepSleepHandler() : sleepScheduledTime(0), sleepScheduled(false), timerWakeupEnabled(false) {}

bool DeepSleepHandler::isPinInHoldList(gpio_num_t pin) const {
    for (const auto& holdPin : holdPins) {
        if (holdPin.pin == pin)
            return true;
    }
    return false;
}

bool DeepSleepHandler::isPinInWakeupList(gpio_num_t pin) const {
    for (const auto& wakeupPin : wakeupPins) {
        if (wakeupPin.pin == pin)
            return true;
    }
    return false;
}

bool DeepSleepHandler::isGpioHoldCapable(gpio_num_t pin) {
    // Only these pins support GPIO hold during deep sleep
    const gpio_num_t holdCapablePins[] = {GPIO_NUM_0,  GPIO_NUM_2,  GPIO_NUM_4,  GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14,
                                          GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33};

    for (const auto& holdPin : holdCapablePins) {
        if (pin == holdPin)
            return true;
    }
    return false;
}

void DeepSleepHandler::addHoldPin(gpio_num_t pin, int value) {
    // Check for conflict with wake-up pins first
    if (isPinInWakeupList(pin)) {
        Serial.printf("WARNING: Pin %d is already configured as wake-up pin! This may cause conflicts.\n", pin);
    }

    // Check if pin supports GPIO hold (more restrictive than just RTC-capable)
    if (!isGpioHoldCapable(pin)) {
        Serial.printf("ERROR: Pin %d does NOT support GPIO hold during sleep!\n", pin);
        Serial.printf("       Hold-capable pins: 0,2,4,12,13,14,15,25,26,27,32,33\n");
        // return; // Do add non-hold-capable pins
    }

    // Check if pin already in hold list
    for (auto& holdPin : holdPins) {
        if (holdPin.pin == pin) {
            Serial.printf("Updating hold pin %d from value %d to %d\n", pin, holdPin.value, value);
            holdPin.value = value;
            return;
        }
    }

    // Add new hold pin
    holdPins.push_back({pin, value});
    Serial.printf("Added hold pin %d with value %d (GPIO hold capable: YES)\n", pin, value);
}

void DeepSleepHandler::addWakeupPin(gpio_num_t pin, WakeupTrigger trigger) {
    // Check for conflict with hold pins
    if (isPinInHoldList(pin)) {
        Serial.printf("WARNING: Pin %d is already configured as hold pin! This may cause conflicts.\n", pin);
    }

    // Check if pin already in wake-up list
    for (auto& wakeupPin : wakeupPins) {
        if (wakeupPin.pin == pin) {
            Serial.printf("Updating wake-up pin %d trigger\n", pin);
            wakeupPin.trigger = trigger;
            return;
        }
    }

    // Add new wake-up pin
    wakeupPins.push_back({pin, trigger});
    const char* triggerStr = "";
    switch (trigger) {
        case WakeupTrigger::WAKE_HIGH:
            triggerStr = "HIGH";
            break;
        case WakeupTrigger::WAKE_LOW:
            triggerStr = "LOW";
            break;
        case WakeupTrigger::WAKE_RISING:
            triggerStr = "RISING";
            break;
        case WakeupTrigger::WAKE_FALLING:
            triggerStr = "FALLING";
            break;
    }
    Serial.printf("Added wake-up pin %d with trigger %s\n", pin, triggerStr);
}

void DeepSleepHandler::setWakeupPins(const std::vector<WakeupPin>& pins) {
    wakeupPins.clear();
    for (const auto& pin : pins) {
        addWakeupPin(pin.pin, pin.trigger);
    }
}

void DeepSleepHandler::setWakeupBitmask(uint64_t bitmask, WakeupTrigger trigger) {
    wakeupPins.clear();
    for (int pin = 0; pin < 40; pin++) {
        if (bitmask & BUTTON_PIN_BITMASK(pin)) {
            addWakeupPin((gpio_num_t)pin, trigger);
        }
    }
}

uint64_t DeepSleepHandler::buildWakeupBitmask() const {
    uint64_t bitmask = 0;
    for (const auto& wakeupPin : wakeupPins) {
        bitmask |= BUTTON_PIN_BITMASK(wakeupPin.pin);
    }
    return bitmask;
}

void DeepSleepHandler::enableTimerWakeup(uint64_t timeInUs) {
    esp_sleep_enable_timer_wakeup(timeInUs);
    timerWakeupEnabled = true;
}

void DeepSleepHandler::disableTimerWakeup() {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    timerWakeupEnabled = false;
}

void DeepSleepHandler::scheduleDeepSleep(long delayMs) {
    sleepScheduledTime = millis() + delayMs;
    sleepScheduled = true;
}

void DeepSleepHandler::cancelScheduledSleep() { sleepScheduled = false; }

bool DeepSleepHandler::shouldSleepNow() const { return sleepScheduled && (millis() >= sleepScheduledTime); }

void DeepSleepHandler::addHighImpedancePin(gpio_num_t pin) {
    HighImpedancePin hiPin;
    hiPin.pin = pin;
    highImpedancePins.push_back(hiPin);

    Serial.printf("Added high impedance pin: GPIO %d\n", pin);
}

void DeepSleepHandler::enterDeepSleep() {
    uint64_t bitmask = buildWakeupBitmask();
    // Use ext1 as a wake-up source
    esp_sleep_enable_ext1_wakeup(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);

    // Waarschijnlijk moet ik van de ADC pinnen eerst nog gewone IO pinnen maken
    for (const auto& wakeupPin : wakeupPins) {
        rtc_gpio_init(wakeupPin.pin);
        rtc_gpio_set_direction(wakeupPin.pin, RTC_GPIO_MODE_INPUT_ONLY);
        // Set pull-up or pull-down based on trigger type
        if (wakeupPin.trigger == WakeupTrigger::WAKE_HIGH || wakeupPin.trigger == WakeupTrigger::WAKE_RISING) {
            rtc_gpio_pullup_dis(wakeupPin.pin);
            rtc_gpio_pulldown_en(wakeupPin.pin);
        } else if (wakeupPin.trigger == WakeupTrigger::WAKE_LOW || wakeupPin.trigger == WakeupTrigger::WAKE_FALLING) {
            rtc_gpio_pulldown_dis(wakeupPin.pin);
            rtc_gpio_pullup_en(wakeupPin.pin);
        }
    }
    // Set all pins to their values, also non-hold-capable pins
    for (const auto& holdPin : holdPins) {
        pinMode(holdPin.pin, OUTPUT);
        digitalWrite(holdPin.pin, holdPin.value);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
    // Enable GPIO hold for all hold-capable pins
    gpio_deep_sleep_hold_en();
    for (const auto& holdPin : holdPins) {
        if (rtc_gpio_is_valid_gpio(holdPin.pin)) {
            gpio_hold_en(holdPin.pin);
            Serial.printf("GPIO hold enabled for pin %d with value %d\n", holdPin.pin, holdPin.value);
        } else {
            Serial.printf("ERROR: Pin %d is not valid for GPIO hold!\n", holdPin.pin);
        }
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
    esp_task_wdt_deinit();  // <--- Add this line to disable the task WDT

    Serial.println("Entering deep sleep...");
    Serial.flush();
    esp_deep_sleep_start();
}

void DeepSleepHandler::handleWakeup() {
    Serial.println("Waking up from deep sleep...");

    // Disable all GPIO holds
    gpio_deep_sleep_hold_dis();
    for (const auto& holdPin : holdPins) {
        if (rtc_gpio_is_valid_gpio(holdPin.pin)) {
            gpio_hold_dis(holdPin.pin);
        }
    }
    // Clear scheduled sleep
    sleepScheduled = false;
}

bool DeepSleepHandler::isWakeFromSleep() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    return (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED);
}

esp_sleep_wakeup_cause_t DeepSleepHandler::getWakeupCause() { return esp_sleep_get_wakeup_cause(); }

void DeepSleepHandler::printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT1: {
            Serial.println("Wakeup caused by external signal using RTC_CNTL (EXT1)");
            uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

            Serial.printf("Wake-up pin mask: 0x%llX\n", wakeup_pin_mask);

            if (wakeup_pin_mask != 0) {
                int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
                Serial.printf("Wake-up pin: %d\n", pin);

                // Debug: Show all pins that are HIGH
                Serial.println("All pin states at wake-up:");
                for (int p = 32; p <= 39; p++) {
                    if ((wakeup_pin_mask & (1ULL << p)) != 0) {
                        Serial.printf("  GPIO %d: HIGH (triggered wake-up)\n", p);
                    } else if (rtc_gpio_is_valid_gpio((gpio_num_t)p)) {
                        int state = gpio_get_level((gpio_num_t)p);
                        Serial.printf("  GPIO %d: %s\n", p, state ? "HIGH" : "LOW");
                    }
                }
            } else {
                Serial.println("ERROR: EXT1 wake-up but no pin mask!");
                Serial.println("This suggests the wake-up was caused by something else.");
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO (EXT0)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;
        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

void DeepSleepHandler::printConfiguration() const {
    Serial.println("=== DeepSleepHandler Configuration ===");
    Serial.println("Hold pins:");
    for (const auto& holdPin : holdPins) {
        Serial.printf("  Pin %d: %s\n", holdPin.pin, holdPin.value ? "HIGH" : "LOW");
    }
    Serial.println("Wake-up pins:");
    for (const auto& wakeupPin : wakeupPins) {
        const char* triggerStr = "";
        switch (wakeupPin.trigger) {
            case WakeupTrigger::WAKE_HIGH:
                triggerStr = "HIGH";
                break;
            case WakeupTrigger::WAKE_LOW:
                triggerStr = "LOW";
                break;
            case WakeupTrigger::WAKE_RISING:
                triggerStr = "RISING";
                break;
            case WakeupTrigger::WAKE_FALLING:
                triggerStr = "FALLING";
                break;
        }
        Serial.printf("  Pin %d: trigger on %s\n", wakeupPin.pin, triggerStr);
    }
}

void DeepSleepHandler::clearAllPins() {
    holdPins.clear();
    wakeupPins.clear();
    Serial.println("All pin configurations cleared");
}