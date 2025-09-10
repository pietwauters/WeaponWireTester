#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WiFi.h>

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

#include "PreferencesWrapper.h"
#include "SettingsManager.h"
#include "WS2812BLedMatrix.h"
#include "WebTerminal.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/task.h"  // Required for vTaskList()
#include "resitancemeasurement.h"
#include "terminal.html.h"
using namespace std;

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "GpioHoldManager.h"
#include "RTOSUtilities.h"
#include "USBSerialTerminal.h"
#include "adc_calibrator.h"
#include "driver/gpio.h"  // Required for gpio_pad_select_gpio()
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "soc/io_mux_reg.h"  // For IO_MUX register definitions

int myRefs_Ohm[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int StoredRefs_ohm[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int R0 = 130;
int Vmax = 2992;
volatile bool DoCalibration = false;
bool MirrorMode = true;              // Default to no mirror mode
int CalibrationDisplayChannel = 0;   // Default to channel 0
bool CalibrationAutoMode = false;    // Auto mode flag
int Brightness = BRIGHTNESS_NORMAL;  // Default brightness level

AsyncWebServer server(80);
SettingsManager settings;
WebTerminal terminal(server);
USBSerialTerminal serialTerminal;  // Add this global variable

WS2812B_LedMatrix* LedPanel;

// Forward declarations
void synchronizeThresholdValues();
void LoadSettings();
void handleEchoCommand(ITerminal* term, const std::vector<String>& args);
void handleRebootCommand(ITerminal* term, const std::vector<String>& args);
void handleHelpCommand(ITerminal* term, const std::vector<String>& args);
void handleCalibrateCommand(ITerminal* term, const std::vector<String>& args);
void handleListCommand(ITerminal* term, const std::vector<String>& args);  // Add this
void handleSetCommand(ITerminal* term, const std::vector<String>& args);   // Add this

// Command handler class declaration
class CommonCommandHandler {
   public:
    template <typename TerminalType>
    void registerTo(TerminalType* terminal) {
        terminal->registerCommand("echo", handleEchoCommand);
        terminal->registerCommand("reboot", handleRebootCommand);
        terminal->registerCommand("calibrate", handleCalibrateCommand);
        terminal->registerCommand("list", handleListCommand);
        terminal->registerCommand("set", handleSetCommand);
        terminal->registerCommand("help", handleHelpCommand);
    }
};

// Create a single global command handler instance
CommonCommandHandler commandHandler;

ITerminal* currentTerminal = nullptr;

void handleCalibrateCommand(ITerminal* term, const std::vector<String>& args) {
    currentTerminal = term;  // Set global pointer
    if (args.empty()) {
        term->printf("Calibrate command requires 'on' or 'off' [channel]\n");
        term->printf("Usage: calibrate on [0-2]  or  calibrate off\n");
        term->printf("Default: auto mode (tracks lowest value channel)\n");
        return;
    }

    if (args[0] == "on") {
        DoCalibration = true;

        // Set which channel to display
        if (args.size() > 1) {
            if (args[1] == "auto") {
                CalibrationAutoMode = true;
                CalibrationDisplayChannel = 0;
                term->printf("Calibration started in AUTO mode (lowest value channel)\n");
            } else {
                int channel = args[1].toInt();
                if (channel >= 0 && channel <= 2) {
                    CalibrationAutoMode = false;
                    CalibrationDisplayChannel = channel;
                    term->printf("Calibration started for channel %d\n", CalibrationDisplayChannel);
                } else {
                    term->printf("Invalid channel %d. Using AUTO mode.\n", channel);
                    CalibrationAutoMode = true;
                    CalibrationDisplayChannel = 0;
                    term->printf("Calibration started in AUTO mode (lowest value channel)\n");
                }
            }
        } else {
            CalibrationAutoMode = true;
            CalibrationDisplayChannel = 0;
            term->printf("Calibration started in AUTO mode (lowest value channel)\n");
        }

    } else if (args[0] == "off") {
        DoCalibration = false;

    } else {
        term->printf("Unknown argument for Calibrate command\n");
    }
}

// Function to synchronize myRefs_Ohm with StoredRefs_ohm after settings changes
void synchronizeThresholdValues() {
    for (int i = 0; i < 11; i++) {
        myRefs_Ohm[i] = StoredRefs_ohm[i];
    }
}

void AdjustThreasholdForRealV() {
    static bool bInitialAdjustmentDone = false;
    static long TimeToTest = 0;

    // Only perform initial adjustment once at startup
    if (bInitialAdjustmentDone) {
        return;
    }

    int Vreal = 3300;
    bool OKToAdjust = true;
    testWiresOnByOne();

    if (millis() < TimeToTest) {
        return;
    }

    for (int j = 0; j < 3; j++) {
        if ((measurements[j][j] > 2500)) {
            if (measurements[j][j] < Vreal) {
                Vreal = measurements[j][j];
            }
        } else {
            OKToAdjust = false;
            j = 3;
        }
    }

    if (OKToAdjust) {
        float factor = (float)Vreal / (float)Vmax;
        for (int i = 0; i < 11; i++) {
            myRefs_Ohm[i] = (int)(StoredRefs_ohm[i] * factor);
        }
        bInitialAdjustmentDone = true;  // Mark as completed
    } else {
        // If conditions aren't met, try again in 1 second
        TimeToTest = millis() + 1000;
    }
}

constexpr int NR_SAMPLES_DIVIDER = 10;
constexpr int NR_SAMPLES = (1L << NR_SAMPLES_DIVIDER);

// Calculate the best representative value for ADC measurements
long calculateBestADCValue(const std::vector<long>& sortedSamples) {
    if (sortedSamples.empty()) {
        return 0;
    }

    // Create frequency map
    std::map<long, int> valueCount;
    for (const auto& val : sortedSamples) {
        valueCount[val]++;
    }

    // Method 1: Weighted average of most frequent triplet (best for stable measurements)
    long bestTripletCenter = sortedSamples[sortedSamples.size() / 2];  // fallback to median
    int bestTripletSum = 0;

    for (const auto& pair : valueCount) {
        long centerValue = pair.first;
        int tripletSum = 0;

        // Sum frequencies of center-1, center, center+1
        auto it = valueCount.find(centerValue - 1);
        if (it != valueCount.end()) {
            tripletSum += it->second;
        }

        tripletSum += pair.second;  // center value
        it = valueCount.find(centerValue + 1);
        if (it != valueCount.end()) {
            tripletSum += it->second;
        }

        if (tripletSum > bestTripletSum) {
            bestTripletSum = tripletSum;
            bestTripletCenter = centerValue;
        }
    }

    // Calculate weighted average of the triplet
    long weightedSum = 0;
    int totalWeight = 0;

    for (long val = bestTripletCenter - 1; val <= bestTripletCenter + 1; val++) {
        auto it = valueCount.find(val);
        if (it != valueCount.end()) {
            weightedSum += val * it->second;
            totalWeight += it->second;
        }
    }

    return (totalWeight > 0) ? (weightedSum / totalWeight) : bestTripletCenter;
}

// Alternative: Trimmed mean (good for general noise reduction)
long calculateTrimmedMean(const std::vector<long>& sortedSamples, float trimPercent = 0.1f) {
    if (sortedSamples.empty()) {
        return 0;
    }

    int trimCount = (int)(sortedSamples.size() * trimPercent / 2);
    if (trimCount >= sortedSamples.size() / 2) {
        trimCount = 0;
    }

    long sum = 0;
    int count = 0;

    for (int i = trimCount; i < sortedSamples.size() - trimCount; i++) {
        sum += sortedSamples[i];
        count++;
    }

    return (count > 0) ? (sum / count) : sortedSamples[sortedSamples.size() / 2];
}

void Calibrate() {
    long value = 0;
    long sample;
    vector<long> SortedSamples[3];

    long max[] = {0, 0, 0};
    long min[] = {9999, 9999, 9999};
    long current_time;
    long duration;

    while (DoCalibration) {
        value = 0;

        esp_task_wdt_reset();
        SortedSamples[0].clear();
        SortedSamples[1].clear();
        SortedSamples[2].clear();
        current_time = millis();

        for (int i = 0; i < NR_SAMPLES; i++) {
            testStraightOnly();
            esp_task_wdt_reset();
            for (int j = 0; j < 3; j++) {
                auto it = std::lower_bound(SortedSamples[j].begin(), SortedSamples[j].end(), measurements[j][j]);
                SortedSamples[j].insert(it, measurements[j][j]);
            }
        }

        duration = (millis() - current_time);
        currentTerminal->printf("Duration = %d\n", duration);

        // Update min/max for all channels (needed for calculations)
        for (int i = 0; i < 3; i++) {
            if (min[i] > SortedSamples[i][0]) {
                min[i] = (SortedSamples[i])[0];
            }

            if (max[i] < SortedSamples[i][NR_SAMPLES - 1]) {
                max[i] = SortedSamples[i][NR_SAMPLES - 1];
            }
        }

        // Auto mode: find channel with lowest median value
        if (CalibrationAutoMode) {
            int bestChannel = 0;
            long lowestMedian = SortedSamples[0][NR_SAMPLES / 2];

            for (int ch = 1; ch < 3; ch++) {
                long median = SortedSamples[ch][NR_SAMPLES / 2];
                if (median < lowestMedian) {
                    lowestMedian = median;
                    bestChannel = ch;
                }
            }

            // Update display channel if it changed
            if (bestChannel != CalibrationDisplayChannel) {
                CalibrationDisplayChannel = bestChannel;
                currentTerminal->printf("AUTO: Switched to channel %d (lowest median: %ld)\n",
                                        CalibrationDisplayChannel, lowestMedian);
            }
        }

        // Display results only for the selected channel
        int i = CalibrationDisplayChannel;

        int percentile_50_index = (int)((long)NR_SAMPLES * 50L) / 100;
        int percentile_90_index = (int)((long)NR_SAMPLES * 90L) / 100;

        // Find most common value
        std::map<long, int> valueCount;
        for (const auto& val : SortedSamples[i]) {
            valueCount[val]++;
        }

        long mostCommonValue = SortedSamples[i][0];
        int maxCount = 0;
        for (const auto& pair : valueCount) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                mostCommonValue = pair.first;
            }
        }

        // Find most frequent 3 consecutive values
        long bestTripletCenter = mostCommonValue;
        int bestTripletSum = 0;

        for (const auto& pair : valueCount) {
            long centerValue = pair.first;
            int tripletSum = 0;

            // Sum frequencies of center-1, center, center+1
            auto it = valueCount.find(centerValue - 1);
            if (it != valueCount.end()) {
                tripletSum += it->second;
            }

            tripletSum += pair.second;  // center value

            it = valueCount.find(centerValue + 1);
            if (it != valueCount.end()) {
                tripletSum += it->second;
            }

            if (tripletSum > bestTripletSum) {
                bestTripletSum = tripletSum;
                bestTripletCenter = centerValue;
            }
        }

        // Find percentile of best triplet center
        int tripletPercentile = 0;
        for (int k = 0; k < SortedSamples[i].size(); k++) {
            if (SortedSamples[i][k] == bestTripletCenter) {
                tripletPercentile = (k * 100) / NR_SAMPLES;
                break;
            }
        }

        // Calculate best ADC values using different methods
        long bestADCValue = calculateBestADCValue(SortedSamples[i]);
        long trimmedMean = calculateTrimmedMean(SortedSamples[i], 0.1f);  // Trim 10% from each end

        // Show mode in the output
        String modeStr = CalibrationAutoMode ? " (AUTO)" : "";
        currentTerminal->printf("Channel %d%s:\n", i, modeStr.c_str());
        currentTerminal->printf("   Min = %d      Max = %d      P50 = %d      P90 = %d\n", SortedSamples[i][0],
                                SortedSamples[i][NR_SAMPLES - 1], SortedSamples[i][percentile_50_index],
                                SortedSamples[i][percentile_90_index]);
        currentTerminal->printf("   Most common = %d (%dx)      Best triplet = %d (%dx) P%d\n", mostCommonValue,
                                maxCount, bestTripletCenter, bestTripletSum, tripletPercentile);
        currentTerminal->printf("   **BEST ADC = %d**      Trimmed mean = %d\n", bestADCValue, trimmedMean);
    }
}

bool CalibrationEnabled;

String deviceName;
void handleListCommand(ITerminal* term, const std::vector<String>& args) {
    term->printf("Available settings:\n");
    term->printf("===================\n");

    term->printf("Integer settings:\n");
    term->printf("  R0                  : %d ohm (Total resistance Ron + 2x47)\n", R0);
    term->printf("  Vmax                : %d mV (Maximum voltage)\n", Vmax);
    term->printf("  Brightness          : %d (Display brightness 1-255)\n", Brightness);

    term->printf("\nBoolean settings:\n");
    term->printf("  bCalibrate          : %s (Perform Calibration?)\n", CalibrationEnabled ? "true" : "false");
    term->printf("  MirrorMode          : %s (Should your LedPanel be mirrored?)\n", MirrorMode ? "true" : "false");

    term->printf("\nString settings:\n");
    term->printf("  name                : %s (Device Name)\n", deviceName.c_str());

    term->printf("\nArray settings:\n");
    term->printf("  myRefs_Ohm          : [");
    for (int i = 0; i < 11; i++) {
        term->printf("%d", StoredRefs_ohm[i]);
        if (i < 10) {
            term->printf(", ");
        }
    }
    term->printf("] (Threshold values 0-10 Ohm)\n");

    term->printf("\nNote: Use the web interface to modify these settings\n");
    term->printf("Current working values:\n");
    term->printf("  myRefs_Ohm (active) : [");
    for (int i = 0; i < 11; i++) {
        term->printf("%d", myRefs_Ohm[i]);
        if (i < 10) {
            term->printf(", ");
        }
    }
    term->printf("]\n");
}

void handleSetCommand(ITerminal* term, const std::vector<String>& args) {
    if (args.size() < 2) {
        term->printf("Usage: set <setting_name> <value>\n");
        term->printf("Available settings: R0, Vmax, bCalibrate, MirrorMode, Brightness, name, myRefs_Ohm\n");
        term->printf("Example: set name \"MyTester\"\n");
        term->printf("Example: set myRefs_Ohm 0,1,2,3,4,5,6,7,8,9,12\n");
        return;
    }

    String settingName = args[0];
    String value = args[1];

    term->printf("Setting '%s' to '%s'...\n", settingName.c_str(), value.c_str());

    // Integer settings
    if (settingName == "R0") {
        int newValue = value.toInt();
        if (newValue <= 0) {
            term->printf("Error: R0 must be > 0\n");
            return;
        }
        R0 = newValue;
        term->printf("✓ Set R0 = %d ohm\n", newValue);

    } else if (settingName == "Vmax") {
        int newValue = value.toInt();
        if (newValue <= 0) {
            term->printf("Error: Vmax must be > 0\n");
            return;
        }
        Vmax = newValue;
        term->printf("✓ Set Vmax = %d mV\n", newValue);
    } else if (settingName == "Brightness") {
        int newValue = value.toInt();
        if (newValue < 1 || newValue > 255) {
            term->printf("Error: Brightness must be between 1 and 255\n");
            return;
        }
        Brightness = newValue;
        term->printf("✓ Set Brightness = %d\n", newValue);

        // Boolean settings
    } else if (settingName == "bCalibrate") {
    } else if (settingName == "MirrorMode") {
        if (value == "true" || value == "1") {
            MirrorMode = true;
            term->printf("✓ Set MirrorMode = true\n");
        } else if (value == "false" || value == "0") {
            MirrorMode = false;
            term->printf("✓ Set MirrorMode = false\n");
        } else {
            term->printf("Error: MirrorMode must be 'true' or 'false'\n");
            return;
        }
        if (value == "true" || value == "1") {
            CalibrationEnabled = true;
            term->printf("✓ Set bCalibrate = true\n");
        } else if (value == "false" || value == "0") {
            CalibrationEnabled = false;
            term->printf("✓ Set bCalibrate = false\n");
        } else {
            term->printf("Error: bCalibrate must be 'true' or 'false'\n");
            return;
        }

        // String settings
    } else if (settingName == "name") {
        // Remove quotes if present
        if (value.startsWith("\"") && value.endsWith("\"")) {
            value = value.substring(1, value.length() - 1);
        }
        deviceName = value;
        term->printf("✓ Set name = \"%s\"\n", value.c_str());

        // Array settings
    } else if (settingName == "myRefs_Ohm") {
        // Parse comma-separated values
        int newRefs[11];
        int count = 0;
        int startPos = 0;

        for (int i = 0; i < 11; i++) {
            int commaPos = value.indexOf(',', startPos);
            String numberStr;

            if (commaPos == -1) {
                // Last number
                numberStr = value.substring(startPos);
            } else {
                numberStr = value.substring(startPos, commaPos);
                startPos = commaPos + 1;
            }

            newRefs[i] = numberStr.toInt();
            count++;

            if (commaPos == -1) {
                break;  // No more commas
            }
        }

        if (count != 11) {
            term->printf("Error: myRefs_Ohm requires exactly 11 values\n");
            return;
        }

        // Validate values are in ascending order
        for (int i = 1; i < 11; i++) {
            if (newRefs[i] < newRefs[i - 1]) {
                term->printf("Error: Values must be in ascending order\n");
                return;
            }
        }

        // Update both stored and working arrays
        for (int i = 0; i < 11; i++) {
            StoredRefs_ohm[i] = newRefs[i];
            myRefs_Ohm[i] = newRefs[i];
        }

        term->printf("✓ Set myRefs_Ohm = [");
        for (int i = 0; i < 11; i++) {
            term->printf("%d", newRefs[i]);
            if (i < 10) {
                term->printf(",");
            }
        }
        term->printf("]\n");

    } else {
        term->printf("Error: Unknown setting '%s'\n", settingName.c_str());
        term->printf("Available:  R0, Vmax, bCalibrate, MirrorMode, Brightness, name, myRefs_Ohm\n");
        return;
    }

    // Save to flash with feedback
    term->printf("Saving to flash...\n");
    settings.save();
    term->printf("✓ Settings successfully saved to flash!\n");
    term->printf("✓ Setting change complete.\n");
}

void LoadSettings() {
    // Register settings

    settings.addBool("bCalibrate", "Perform Calibration?", &CalibrationEnabled);
    settings.addIntArray("myRefs_Ohm", "Threshold values from 0 - 10 Ohm", StoredRefs_ohm, 11);
    settings.addInt("R0", "R0 (total resistance (Ron + 2 x 47)", &R0);
    settings.addInt("Vmax", "Vmax in mV", &Vmax);
    settings.addBool("MirrorMode", "Should your LedPanel be mirrored?", &MirrorMode);
    settings.addInt("Brightness", "Display brightness 1-255", &Brightness);
    settings.addString("name", "Device Name", &deviceName);
    settings.begin("Settings");  // for Preferences namespace
    settings.load();

    // Copy the loaded stored references to working references (AFTER settings.load())
    for (int i = 0; i < 11; i++) {
        myRefs_Ohm[i] = StoredRefs_ohm[i];
    }
}

void SetupNetworkStuff() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Tester", "01041967");
    ElegantOTA.begin(&server);
    Serial.println(WiFi.softAPIP());
    terminal.begin();

    // Register the single command handler to web terminal
    commandHandler.registerTo(&terminal);

    server.begin();
    terminal.printf("HTTP server started\n");

    settings.addWebEndpoints(server);
    settings.setPostSaveCallback(synchronizeThresholdValues);
}

void setupSerialTerminal() {
    serialTerminal.begin();

    // Register the same command handler to serial terminal
    commandHandler.registerTo(&serialTerminal);
}

// Move the command handler implementations here (before they're used)
void handleEchoCommand(ITerminal* term, const std::vector<String>& args) {
    String response;
    for (auto& arg : args) {
        response += arg + " ";
    }
    term->send(response);
}

void handleRebootCommand(ITerminal* term, const std::vector<String>& args) {
    term->send("Rebooting...");
    ESP.restart();
}

void handleHelpCommand(ITerminal* term, const std::vector<String>& args) {
    term->send("Available commands:");
    term->send("  echo <text>          - Echo back the text");
    term->send("  reboot               - Restart the device");
    term->send("  calibrate on [0-2]   - Start calibration (optional channel)");
    term->send("  calibrate on auto    - Start auto calibration (tracks lowest channel)");
    term->send("  calibrate off        - Stop calibration");
    term->send("  list                 - Show available settings");
    term->send("  set <name> <value>   - Change a setting");
    term->send("  help                 - Show this help message");
}

#include "tester.h"

// Global tester instance
Tester* tester = nullptr;
EmpiricalResistorCalibrator mycalibrator;
void setup() {
    // put your setup code here, to run once:
    setCpuFrequencyMhz(240);  // Set CPU frequency to 240 MHz
    Serial.begin(115200);

    LoadSettings();
    LedPanel = new WS2812B_LedMatrix();
    LedPanel->setMirrorMode(MirrorMode);
    LedPanel->begin();
    LedPanel->ClearAll();
    LedPanel->SequenceTest();
    LedPanel->ConfigureBlinking(12, LedPanel->m_Orange, 120, 1000, 0);
    LedPanel->SetBrightness((uint8_t)Brightness);  // Set the brightness level for the LED panel

    // Setup serial terminal first, before other initialization
    setupSerialTerminal();

    esp_task_wdt_init(20, true);
    esp_task_wdt_add(NULL);
    Serial.printf("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
    init_AD();
    Set_IODirectionAndValue(IODirection_br_bl, IOValues_br_bl);
    mycalibrator.begin(br_analog, bl_analog);
    // Try to load existing calibration
    if (!mycalibrator.load_calibration_from_nvs()) {
        // No existing calibration, run interactive calibration
        if (mycalibrator.calibrate_interactively_empirical()) {
            mycalibrator.save_calibration_to_nvs();
        }
    }
    testWiresOnByOne();
    SetupNetworkStuff();

    // Create the tester instance
    tester = new Tester(LedPanel);

    // Start the tester task
    tester->begin();

    // Perform initial threshold adjustment after ADC initialization and settings load
    AdjustThreasholdForRealV();
    for (int i = 0; i < 11; i++) {
        myRefs_Ohm[i] = mycalibrator.get_adc_threshold_for_resistance_with_leads(1.0 * i, 0.0);
        // printf("myRefs_Ohm[%d] = %d     ;    StoredRefs_ohm[%d] = %d\n", i, myRefs_Ohm[i], i, StoredRefs_ohm[i]);
        // fflush(stdout);                 // Force flush
        // vTaskDelay(pdMS_TO_TICKS(10));  // Small delay
    }

    // Set the reference values after loading settings
    tester->setReferenceValues(myRefs_Ohm);  // Pass the int array

    // Remove the idle task from the WDT (do this for both cores)
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
}

void loop() {
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Always run serial terminal
    serialTerminal.loop();
    esp_task_wdt_reset();
    ElegantOTA.loop();
    esp_task_wdt_reset();
    terminal.loop();
    esp_task_wdt_reset();
}

extern "C" void app_main() {
    // Call Arduino setup and loop
    initArduino();  // Initialize Arduino if needed
    setup();
    // printTasks(); // Print tasks during setup      // Call the Arduino setup function
    while (true) {
        loop();  // Call the Arduino loop function
    }
}