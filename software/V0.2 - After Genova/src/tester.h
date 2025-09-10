#pragma once

#include <Arduino.h>

#include "WS2812BLedMatrix.h"
#include "esp_task_wdt.h"
#include "resitancemeasurement.h"

// State enum
typedef enum { Waiting, EpeeTesting, FoilTesting, LameTesting, WireTesting_1, WireTesting_2 } State_t;
typedef enum { SHAPE_F, SHAPE_E, SHAPE_S, SHAPE_P, SHAPE_DIAMOND, SHAPE_SQUARE, SHAPE_NONE } Shapes_t;

// Timeout constants
constexpr int WIRE_TEST_1_TIMEOUT = 50;
constexpr int NO_WIRES_PLUGGED_IN_TIMEOUT = 2;
constexpr int FOIL_TEST_TIMEOUT = 1000;
constexpr int WIRE_TEST_DELAY = 2000;  // 2 seconds delay after special test exit

class Tester {
   private:
    // State variables
    State_t currentState;
    int timeToSwitch;
    int noWireTimeout;
    bool allGood;
    unsigned long lastSpecialTestExit;
    Shapes_t ShowingShape = SHAPE_NONE;

    // Task handle
    TaskHandle_t testerTaskHandle;

    // LED Panel reference
    WS2812B_LedMatrix* ledPanel;

    // Add reference values as class members (correct type: int)
    int* myRefs_Ohm;  // Pointer to reference values

    // Private methods
    void doCommonReturnFromSpecialMode();
    bool delayAndTestWirePluggedIn(long delay);
    bool delayAndTestWirePluggedInFoil(long delay);
    bool delayAndTestWirePluggedInEpee(long delay);
    void doEpeeTest();
    void doFoilTest();
    void doLameTest();
    bool animateSingleWire(int wireIndex);
    bool doQuickCheck();
    void handleWaitingState();
    void handleWireTestingState1();
    void handleWireTestingState2();
    bool debouncedCondition(std::function<bool()> condition, int debounceMs = 10);

    // Static task wrapper
    static void testerTaskWrapper(void* parameter);

   public:
    // Constructor
    Tester(WS2812B_LedMatrix* ledPanelRef);

    // Destructor
    ~Tester();

    // Public methods
    void begin();
    void stop();
    State_t getState() const;
    void setState(State_t newState);
    bool isAllGood() const;

    // Calibration control
    void startCalibration();
    void stopCalibration();

    // Main task loop
    void taskLoop();

    // Add method to set reference values (correct type: int)
    void setReferenceValues(int* refs);

    // Add method to get reference values (for debugging)
    int* getReferenceValues() const;
};

// Global instance declaration
extern Tester* testerInstance;
