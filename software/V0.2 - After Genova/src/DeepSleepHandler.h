#pragma once

#include <vector>

#include "driver/gpio.h"
#include "esp_sleep.h"

enum class WakeupTrigger {
    WAKE_HIGH,
    WAKE_LOW,
    WAKE_RISING,  // Changed from RISING
    WAKE_FALLING  // Changed from FALLING
};

struct HoldPin {
    gpio_num_t pin;
    int value;  // 0 = LOW, 1 = HIGH
};

struct WakeupPin {
    gpio_num_t pin;
    WakeupTrigger trigger;
};

struct HighImpedancePin {
    gpio_num_t pin;
};

class DeepSleepHandler {
   private:
    std::vector<HoldPin> holdPins;                    // Pins to hold during sleep
    std::vector<WakeupPin> wakeupPins;                // Pins to monitor for wake-up
    std::vector<HighImpedancePin> highImpedancePins;  // NEW: Add this vector
    long sleepScheduledTime;                          // When to enter sleep
    bool sleepScheduled;                              // Is sleep scheduled?
    bool timerWakeupEnabled;                          // Is timer wake-up enabled?

    // Helper functions
    bool isPinInHoldList(gpio_num_t pin) const;
    bool isPinInWakeupList(gpio_num_t pin) const;
    uint64_t buildWakeupBitmask() const;
    bool isGpioHoldCapable(gpio_num_t pin);  // Add this line

   public:
    // Constructor
    DeepSleepHandler();

    // Pin management
    void addHoldPin(gpio_num_t pin, int value);                // Hold pin at specific value during sleep
    void setWakeupPins(const std::vector<WakeupPin>& pins);    // Set all wake-up pins at once
    void addWakeupPin(gpio_num_t pin, WakeupTrigger trigger);  // Add single wake-up pin
    void addHighImpedancePin(gpio_num_t pin);                  // NEW: Add method to configure high impedance pins

    // Alternative: Set wake-up pins with bitmask (for compatibility)
    void setWakeupBitmask(uint64_t bitmask, WakeupTrigger trigger = WakeupTrigger::WAKE_HIGH);

    // Timer wake-up
    void enableTimerWakeup(uint64_t timeInUs);
    void disableTimerWakeup();

    // Sleep scheduling
    void scheduleDeepSleep(long delayMs = 0);
    void cancelScheduledSleep();
    bool shouldSleepNow() const;

    // Sleep execution
    void enterDeepSleep();

    // Wake-up handling
    static bool isWakeFromSleep();
    static esp_sleep_wakeup_cause_t getWakeupCause();
    void handleWakeup();

    // Diagnostics
    static void printWakeupReason();
    void printConfiguration() const;

    // Clear all configurations
    void clearAllPins();
};
