#include "tester.h"

#include "globals.h"  // For DoCalibration and other globals

// Global instance
Tester* testerInstance = nullptr;

Tester::Tester(WS2812B_LedMatrix* ledPanelRef)
    : ledPanel(ledPanelRef),
      currentState(Waiting),
      timeToSwitch(WIRE_TEST_1_TIMEOUT),
      noWireTimeout(NO_WIRES_PLUGGED_IN_TIMEOUT),
      allGood(true),
      lastSpecialTestExit(0),
      testerTaskHandle(nullptr),
      myRefs_Ohm(nullptr)  // Initialize to nullptr
{
    testerInstance = this;
}

Tester::~Tester() {
    stop();
    testerInstance = nullptr;
}

void Tester::begin() {
    // Create the tester task
    xTaskCreatePinnedToCore(testerTaskWrapper, "TesterTask",
                            8192,  // Stack size
                            this,  // Parameter passed to task
                            1,     // Priority
                            &testerTaskHandle,
                            1  // Core ID
    );
}

void Tester::stop() {
    if (testerTaskHandle != nullptr) {
        vTaskDelete(testerTaskHandle);
        testerTaskHandle = nullptr;
    }
}

void Tester::testerTaskWrapper(void* parameter) {
    Tester* tester = static_cast<Tester*>(parameter);
    tester->taskLoop();
}

void Tester::taskLoop() {
    // Add watchdog for this task
    esp_task_wdt_add(NULL);

    while (true) {
        if (DoCalibration) {
            ledPanel->ClearAll();
            Calibrate();
        }

        esp_task_wdt_reset();

        switch (currentState) {
            case Waiting:
                handleWaitingState();
                break;

            case WireTesting_1:
                handleWireTestingState1();
                break;

            case WireTesting_2:
                handleWireTestingState2();
                break;

            default:
                // Should not reach here
                currentState = Waiting;
                break;
        }

        esp_task_wdt_reset();
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent watchdog issues
    }
}

void Tester::handleWaitingState() {
    // Always update measurements first
    testWiresOnByOne();
    ledPanel->Blink();

    // Check for special test modes
    if (testArCr() < 160) {
        currentState = EpeeTesting;
        doEpeeTest();
        doCommonReturnFromSpecialMode();
        lastSpecialTestExit = millis();
    } else if (testArBr() < 160) {
        ledPanel->ClearAll();
        doFoilTest();
        doCommonReturnFromSpecialMode();
        lastSpecialTestExit = millis();
    } else if (testBrCr() < 160) {
        doLameTest();
        doCommonReturnFromSpecialMode();
        lastSpecialTestExit = millis();
    }

    esp_task_wdt_reset();

    // Check for wire testing mode with delay after special tests
    if (WirePluggedIn()) {
        // Check if enough time has passed since last special test exit
        if (lastSpecialTestExit == 0 || (millis() - lastSpecialTestExit) > WIRE_TEST_DELAY) {
            currentState = WireTesting_1;
            noWireTimeout = NO_WIRES_PLUGGED_IN_TIMEOUT;
            timeToSwitch = WIRE_TEST_1_TIMEOUT;
        }
        // If not enough time has passed, stay in Waiting mode
    }
}

void Tester::handleWireTestingState1() {
    testWiresOnByOne();
    allGood = doQuickCheck();

    if (allGood) {
        timeToSwitch--;
    } else {
        timeToSwitch = WIRE_TEST_1_TIMEOUT;
        if (!WirePluggedIn()) {
            noWireTimeout--;
        } else {
            noWireTimeout = NO_WIRES_PLUGGED_IN_TIMEOUT;
        }
        if (!noWireTimeout) {
            currentState = Waiting;
        }
    }

    if (!timeToSwitch) {
        for (int i = 0; i < 5; i += 2) {
            ledPanel->SetLine(i, ledPanel->m_Green);
        }
        // This is the time to update the threasholds with the lead resistance
        ledPanel->myShow();
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_task_wdt_reset();
        currentState = WireTesting_2;
    }
}


// Wiretesting2 is only looking for breaks. Resistances have been checked in phase 1
// So I'm using a relatively high and fixed value
void Tester::handleWireTestingState2() {
    for (int i = 100000; i > 0; i--) {
        esp_task_wdt_reset();
        if (!testStraightOnly(160)) {
            i = 0;
        }
    }

    ledPanel->ClearAll();
    ledPanel->myShow();

    for (int i = 0; i < 3; i++) {
        allGood &= animateSingleWire(i);
    }

    esp_task_wdt_reset();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_task_wdt_reset();

    timeToSwitch = 3;
    ledPanel->ClearAll();
    ledPanel->myShow();
    currentState = Waiting;
}

void Tester::doCommonReturnFromSpecialMode() {
    currentState = Waiting;
    esp_task_wdt_reset();
    ledPanel->ClearAll();
    ledPanel->RestartBlink();

    for (int i = 0; i < 25; i++) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        ledPanel->Blink();
        esp_task_wdt_reset();
    }

    ledPanel->ClearAll();
    testWiresOnByOne();
}

bool Tester::delayAndTestWirePluggedIn(long delay) {
    long returnTime = millis() + delay;
    while (millis() < returnTime) {
        esp_task_wdt_reset();
        testWiresOnByOne();
        if (WirePluggedIn()) {
            return true;
        }
    }
    return false;
}

bool Tester::delayAndTestWirePluggedInFoil(long delay) {
    long returnTime = millis() + delay;
    while (millis() < returnTime) {
        esp_task_wdt_reset();
        testWiresOnByOne();
        if (WirePluggedInFoil()) {
            return true;
        }
    }
    return false;
}

bool Tester::delayAndTestWirePluggedInEpee(long delay) {
    long returnTime = millis() + delay;
    while (millis() < returnTime) {
        esp_task_wdt_reset();
        testWiresOnByOne();
        if (WirePluggedInEpee()) {
            return true;
        }
    }
    return false;
}

void Tester::doEpeeTest() {
    int BrCl;
    testWiresOnByOne();
    ShowingShape = SHAPE_NONE;
    LedPanel->ClearAll();
    while (!WirePluggedInEpee()) {
        esp_task_wdt_reset();
        LedPanel->myShow();
        BrCl = testBrCl();
        if (BrCl < 500) {
            if (SHAPE_P != ShowingShape) {
                LedPanel->ClearAll();
                ShowingShape = SHAPE_P;
            }
            if (BrCl < myRefs_Ohm[5]) {
                LedPanel->Draw_P(LedPanel->m_Green);
            } else {
                if (BrCl < myRefs_Ohm[8]) {
                    LedPanel->Draw_P(LedPanel->m_Yellow);
                } else {
                    LedPanel->Draw_P(LedPanel->m_Red);
                }
            }
            LedPanel->myShow();
            if (delayAndTestWirePluggedInFoil(100)) {
                Serial.println("Wire plugged in during foil light, breaking out");
                break;
            }
            continue;
        }

        int arCr = testArCr();
        int arCl = testArCl();
        int arBr = testArBr();
        int brCr = testBrCr();

        // Case 1: Both ArBr and BrCr > 1500, ArCl > 600
        if ((arBr > 1500 && brCr > 1500) && arCl > 600) {
            // Show color based on ArCr
            if (SHAPE_SQUARE != ShowingShape) {
                LedPanel->ClearAll();
                ShowingShape = SHAPE_SQUARE;
            }
            if (arCr < myRefs_Ohm[2]) {
                LedPanel->SetInner9(LedPanel->m_Green);
            } else if (arCr < myRefs_Ohm[4]) {
                LedPanel->SetInner9(LedPanel->m_Yellow);
            } else if (arCr < 600) {
                LedPanel->SetInner9(LedPanel->m_Red);
            } else {
                // Above 600, don't show anything
                ShowingShape = SHAPE_NONE;
                LedPanel->ClearAll();
                LedPanel->Draw_E(LedPanel->m_White);
                LedPanel->myShow();
                testWiresOnByOne();
                continue;
            }
            LedPanel->myShow();
            if (delayAndTestWirePluggedInEpee(1000)) {
                break;
            }
            // LedPanel->ClearAll();
            LedPanel->myShow();
        }
        // Case 2: Both ArBr and BrCr > 1500, ArCl < 600
        else if ((arBr > 1500 && brCr > 1500) && arCl < 600) {
            // Use thresholds divided by 2
            if (arCr < myRefs_Ohm[1]) {
                LedPanel->SetInner9(LedPanel->m_Green);
            } else if (arCr < myRefs_Ohm[2]) {
                LedPanel->SetInner9(LedPanel->m_Yellow);
            } else if (arCr < 300) {
                LedPanel->SetInner9(LedPanel->m_Red);
            } else {
                LedPanel->ClearAll();
                LedPanel->Draw_E(LedPanel->m_White);
                LedPanel->myShow();
                testWiresOnByOne();
                continue;
            }
            LedPanel->myShow();
            if (delayAndTestWirePluggedInEpee(1000)) {
                break;
            }
            // LedPanel->ClearAll();
            LedPanel->myShow();
        }
        // Case 3: ArBr < 1500 or BrCr < 1500 (unwanted short)
        else if (arBr < 1500 || brCr < 1500) {
            LedPanel->ClearAll();
            if (arBr < 1500) {
                LedPanel->AnimateArBrConnection();
            }
            if (brCr < 1500) {
                LedPanel->AnimateBrCrConnection();
            }
        }
        // All other cases: draw white E
        else {
            LedPanel->ClearAll();
            LedPanel->Draw_E(LedPanel->m_White);
            LedPanel->myShow();
        }

        esp_task_wdt_reset();
        testWiresOnByOne();
    }
    LedPanel->ClearAll();
    LedPanel->myShow();
}

void Tester::doFoilTest() {
    testWiresOnByOne();
    int BrCl;

    while (!WirePluggedInFoil()) {
        esp_task_wdt_reset();
        LedPanel->myShow();
        BrCl = testBrCl();
        if (BrCl < 500) {
            if (SHAPE_P != ShowingShape) {
                LedPanel->ClearAll();
                ShowingShape = SHAPE_P;
            }
            if (BrCl < myRefs_Ohm[5]) {
                LedPanel->Draw_P(LedPanel->m_Green);
            } else {
                if (BrCl < myRefs_Ohm[8]) {
                    LedPanel->Draw_P(LedPanel->m_Yellow);
                } else {
                    LedPanel->Draw_P(LedPanel->m_Red);
                }
            }
            LedPanel->myShow();
            if (delayAndTestWirePluggedInFoil(100)) {
                Serial.println("Wire plugged in during foil light, breaking out");
                break;
            }
            continue;
        }

        int arBr = testArBr();
        if (SHAPE_F != ShowingShape) {
            LedPanel->ClearAll();
            ShowingShape = SHAPE_F;
        }
        if (arBr < myRefs_Ohm[2]) {
            LedPanel->Draw_F(LedPanel->m_Green);
            testWiresOnByOne();
            continue;
        } else if (arBr < myRefs_Ohm[4]) {
            LedPanel->Draw_F(LedPanel->m_Yellow);
            testWiresOnByOne();
            continue;
        } else if (arBr < 2000) {
            LedPanel->Draw_F(LedPanel->m_Red);
            testWiresOnByOne();
            continue;
        } else {
            // Debounce: testArBr() > 1500 must be true for 10ms
            bool debounced = true;
            unsigned long start = millis();
            while (millis() - start < 10) {
                esp_task_wdt_reset();
                if (testArBr() <= 1500) {
                    debounced = false;
                    break;
                }
                taskYIELD();
            }

            if (!debounced) {
                Serial.println("Debouncing failed, continuing loop");
                continue;
            }

            // Debouncing succeeded: show inner lights based on ArCl
            if (SHAPE_SQUARE != ShowingShape) {
                LedPanel->ClearAll();
                ShowingShape = SHAPE_SQUARE;
            }
            if (testArCl() < myRefs_Ohm[1]) {
                LedPanel->SetInner9(LedPanel->m_Green);
            } else if (testArCl() < myRefs_Ohm[2]) {
                LedPanel->SetInner9(LedPanel->m_Yellow);
            } else if (testArCl() < 500) {
                LedPanel->SetInner9(LedPanel->m_Red);
            } else {
                LedPanel->SetInner9(LedPanel->m_White);
            }
            LedPanel->myShow();

            if (delayAndTestWirePluggedInFoil(1000)) {
                Serial.println("Wire plugged in during foil light, breaking out");
                break;
            }
            if (testArBr() <= 1500) {
                LedPanel->ClearAll();
                LedPanel->myShow();
            }
        }

        testWiresOnByOne();
    }

    Serial.println("Wire plugged in during foil test, leaving");
    LedPanel->ClearAll();
    LedPanel->myShow();
}

void Tester::doLameTest() {
    // Your existing DoLameTest code
    bool bShowingRed = false;
    testWiresOnByOne();
    while (!WirePluggedIn()) {
        esp_task_wdt_reset();
        if (testBrCr() < myRefs_Ohm[5]) {
            LedPanel->DrawDiamond(LedPanel->m_Green);
            bShowingRed = false;
            // while((testBrCr()<myRefs_Ohm[5])){esp_task_wdt_reset();};
            while (debouncedCondition([this]() { return testBrCr() < myRefs_Ohm[5]; }, 10));
        } else {
            if (testBrCr() < myRefs_Ohm[10]) {
                LedPanel->DrawDiamond(LedPanel->m_Yellow);
                bShowingRed = false;
                // while((testBrCr()<myRefs_Ohm[10])){esp_task_wdt_reset();};
                while (debouncedCondition([this]() { return testBrCr() < myRefs_Ohm[10]; }, 10));
            }
        }

        if (!bShowingRed)
            LedPanel->DrawDiamond(LedPanel->m_Red);
        bShowingRed = true;
        esp_task_wdt_reset();

        if (delayAndTestWirePluggedIn(250))
            break;
        esp_task_wdt_reset();
        testWiresOnByOne();
    }
    LedPanel->ClearAll();
    LedPanel->myShow();
}

bool Tester::animateSingleWire(int wireIndex) {
    // Your existing AnimateSingleWire code
    bool bOK = false;
    if (measurements[wireIndex][wireIndex] < myRefs_Ohm[10]) {
        if ((measurements[wireIndex][(wireIndex + 1) % 3] > 200) &&
            (measurements[wireIndex][(wireIndex + 2) % 3] > 200)) {
            // OK
            int level = 2;
            if (measurements[wireIndex][wireIndex] <= myRefs_Ohm[3])
                level = 1;
            if (measurements[wireIndex][wireIndex] <= myRefs_Ohm[1])
                level = 0;

            LedPanel->AnimateGoodConnection(wireIndex, level);
            bOK = true;
        } else {
            // short
            if (measurements[wireIndex][(wireIndex + 1) % 3] < 160)
                LedPanel->AnimateShort(wireIndex, (wireIndex + 1) % 3);
            else if (measurements[wireIndex][(wireIndex + 2) % 3] < 160)
                LedPanel->AnimateShort(wireIndex, (wireIndex + 2) % 3);
        }
    } else {
        if ((measurements[wireIndex][(wireIndex + 1) % 3] > 160) &&
            (measurements[wireIndex][(wireIndex + 2) % 3] > 160)) {
            // Simply broken
            LedPanel->AnimateBrokenConnection(wireIndex);
        } else {
            if (measurements[wireIndex][(wireIndex + 1) % 3] < 160)
                LedPanel->AnimateWrongConnection(wireIndex, (wireIndex + 1) % 3);
            if (measurements[wireIndex][(wireIndex + 2) % 3] < 160)
                LedPanel->AnimateWrongConnection(wireIndex, (wireIndex + 2) % 3);
        }
    }
    return bOK;
}

bool Tester::doQuickCheck() {
    // Your existing DoQuickCheck code
    bool bAllGood = true;
    testWiresOnByOne();
    for (int i = 0; i < 3; i++) {
        bAllGood &= animateSingleWire(i);
    }
    esp_task_wdt_reset();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_task_wdt_reset();
    ledPanel->ClearAll();
    esp_task_wdt_reset();
    vTaskDelay(300 / portTICK_PERIOD_MS);
    esp_task_wdt_reset();
    return bAllGood;
}

// Public getter methods
State_t Tester::getState() const { return currentState; }

void Tester::setState(State_t newState) { currentState = newState; }

bool Tester::isAllGood() const { return allGood; }

void Tester::startCalibration() { DoCalibration = true; }

void Tester::stopCalibration() { DoCalibration = false; }

void Tester::setReferenceValues(int* refs) { myRefs_Ohm = refs; }

int* Tester::getReferenceValues() const { return myRefs_Ohm; }

bool Tester::debouncedCondition(std::function<bool()> condition, int debounceMs) {
    unsigned long falseStartTime = 0;
    bool timing = false;

    while (true) {
        bool currentCondition = condition();

        if (currentCondition) {
            // Condition is true, reset timer and keep waiting
            timing = false;
            falseStartTime = 0;
        } else {
            if (!timing) {
                // Just became false, start timing
                falseStartTime = millis();
                timing = true;
            }
            // Check if it's been false long enough
            if (millis() - falseStartTime >= debounceMs) {
                return false;  // Debounced: condition has been false for debounceMs
            }
        }
        esp_task_wdt_reset();
        taskYIELD();
    }
}
