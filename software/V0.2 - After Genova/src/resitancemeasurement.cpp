#include <Arduino.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"

// Number of ADC samples to take for each measurement
#define NUM_ADC_SAMPLES 16

// If you have different pins, change below defines

// Below table uses AD channels and not pin numbers
#define cl_analog ADC1_CHANNEL_0
#define bl_analog ADC1_CHANNEL_3
#define piste_analog ADC1_CHANNEL_6
#define cr_analog ADC1_CHANNEL_7
#define br_analog ADC1_CHANNEL_4
#define ar_analog ADC1_CHANNEL_5

#define al_driver 33
#define bl_driver 21
#define cl_driver 23
#define ar_driver 25
#define br_driver 05
#define cr_driver 18
#define piste_driver 19

// below defines are generated with the excel tool

#define IODirection_ar_br 231
#define IODirection_ar_cr 215
#define IODirection_ar_piste 183
#define IODirection_ar_bl 245
#define IODirection_ar_cl 243
#define IODirection_al_br 238
#define IODirection_al_cr 222
#define IODirection_al_piste 190
#define IODirection_al_bl 252
#define IODirection_al_cl 250
#define IODirection_br_cr 207
#define IODirection_br_bl 237
#define IODirection_br_cl 235
#define IODirection_br_piste 175
#define IODirection_bl_cl 249
#define IODirection_bl_piste 189
#define IODirection_bl_cr 221
#define IODirection_cr_piste 159
#define IODirection_cr_cl 219
#define IODirection_cl_piste 187
#define IODirection_cr_bl 221

#define IOValues_ar_br 8
#define IOValues_ar_cr 8
#define IOValues_ar_piste 8
#define IOValues_ar_bl 8
#define IOValues_ar_cl 8
#define IOValues_al_br 1
#define IOValues_al_cr 1
#define IOValues_al_piste 1
#define IOValues_al_bl 1
#define IOValues_al_cl 1
#define IOValues_br_cr 16
#define IOValues_br_bl 16
#define IOValues_br_cl 16
#define IOValues_br_piste 16
#define IOValues_bl_cl 2
#define IOValues_bl_piste 2
#define IOValues_bl_cr 2
#define IOValues_cr_piste 32
#define IOValues_cr_cl 32
#define IOValues_cl_piste 4

#define IOValues_cr_bl 32

const uint8_t driverpins[] = {al_driver, bl_driver, cl_driver, ar_driver, br_driver, cr_driver, piste_driver};
int measurements[3][3];

void Set_IODirectionAndValue(uint8_t setting, uint8_t values) {
    uint8_t mask = 1;
    for (int i = 0; i < 7; i++) {
        if (setting & mask) {
            pinMode(driverpins[i], INPUT);
        } else {
            pinMode(driverpins[i], OUTPUT);
            if (values & mask) {
                digitalWrite(driverpins[i], HIGH);
            } else {
                digitalWrite(driverpins[i], LOW);
            }
        }
        mask <<= 1;
    }
}

esp_adc_cal_characteristics_t adc_chars;

// Forward declarations
// void calibrateADCOffsets();
int getCalibratedVoltage(int raw_value, adc1_channel_t channel);

/*
int getSample(adc1_channel_t pin){
  int max = 0;
  int sample;
  for(int i = 0; i< 7; i++){
      //sample = analogReadMilliVolts(pin);
      sample = adc1_get_raw(pin);
      if(sample > max)
        max =sample;
  }
  return esp_adc_cal_raw_to_voltage(max, &adc_chars);
}
*/

int getDifferentialSample(adc1_channel_t pin1, adc1_channel_t pin2) {
    int samples1[NUM_ADC_SAMPLES];
    int samples2[NUM_ADC_SAMPLES];

    // Collect NUM_ADC_SAMPLES samples from each pin
    for (int i = 0; i < NUM_ADC_SAMPLES; i++) {
        esp_task_wdt_reset();
        samples1[i] = adc1_get_raw(pin1);
        esp_task_wdt_reset();
        samples2[i] = adc1_get_raw(pin2);
    }

    // Simple insertion sort for NUM_ADC_SAMPLES elements (very fast)
    for (int i = 1; i < NUM_ADC_SAMPLES; i++) {
        int key1 = samples1[i];
        int key2 = samples2[i];
        int j = i - 1;
        while (j >= 0 && samples1[j] > key1) {
            samples1[j + 1] = samples1[j];
            samples2[j + 1] = samples2[j];
            j--;
        }
        samples1[j + 1] = key1;
        samples2[j + 1] = key2;
    }

    // Use proper median values (for even number of samples, average the two middle values)
    int median1, median2;
    if (NUM_ADC_SAMPLES % 2 == 0) {
        // Even number: average the two middle elements
        median1 = (samples1[NUM_ADC_SAMPLES / 2 - 1] + samples1[NUM_ADC_SAMPLES / 2]) / 2;
        median2 = (samples2[NUM_ADC_SAMPLES / 2 - 1] + samples2[NUM_ADC_SAMPLES / 2]) / 2;
    } else {
        // Odd number: take the middle element
        median1 = samples1[NUM_ADC_SAMPLES / 2];
        median2 = samples2[NUM_ADC_SAMPLES / 2];
    }

    int delta = (getCalibratedVoltage(median1, pin1) - getCalibratedVoltage(median2, pin2));
    return delta;
}

// Debug version to understand what's happening
int getDifferentialSampleDebug(adc1_channel_t pin1, adc1_channel_t pin2) {
    int samples1[NUM_ADC_SAMPLES];
    int samples2[NUM_ADC_SAMPLES];

    // Collect NUM_ADC_SAMPLES samples from each pin
    for (int i = 0; i < NUM_ADC_SAMPLES; i++) {
        esp_task_wdt_reset();
        samples1[i] = adc1_get_raw(pin1);
        esp_task_wdt_reset();
        samples2[i] = adc1_get_raw(pin2);
    }

    // Simple insertion sort for NUM_ADC_SAMPLES elements (very fast)
    for (int i = 1; i < NUM_ADC_SAMPLES; i++) {
        int key1 = samples1[i];
        int key2 = samples2[i];
        int j = i - 1;
        while (j >= 0 && samples1[j] > key1) {
            samples1[j + 1] = samples1[j];
            samples2[j + 1] = samples2[j];
            j--;
        }
        samples1[j + 1] = key1;
        samples2[j + 1] = key2;
    }

    // Use proper median values
    int median1, median2;
    if (NUM_ADC_SAMPLES % 2 == 0) {
        median1 = (samples1[NUM_ADC_SAMPLES / 2 - 1] + samples1[NUM_ADC_SAMPLES / 2]) / 2;
        median2 = (samples2[NUM_ADC_SAMPLES / 2 - 1] + samples2[NUM_ADC_SAMPLES / 2]) / 2;
    } else {
        median1 = samples1[NUM_ADC_SAMPLES / 2];
        median2 = samples2[NUM_ADC_SAMPLES / 2];
    }

    int voltage1 = getCalibratedVoltage(median1, pin1);
    int voltage2 = getCalibratedVoltage(median2, pin2);
    int delta = voltage1 - voltage2;

    // Debug output (comment out after testing)
    /*
    Serial.printf("Pin1 raw median: %d -> %dmV, Pin2 raw median: %d -> %dmV, Delta: %dmV\n",
                  median1, voltage1, median2, voltage2, delta);
    */

    return delta;
}

extern const int Reference_3_Ohm[] = {3 * 16, 3 * 16, 3 * 16};
extern const int Reference_5_Ohm[] = {5 * 16, 5 * 16, 5 * 16};
extern const int Reference_10_Ohm[] = {10 * 16, 10 * 16, 10 * 16};

uint8_t testsettings[][3][2] = {
    {{IODirection_cr_cl, IOValues_cr_cl},
     {IODirection_cr_piste, IOValues_cr_piste},
     {IODirection_cr_bl, IOValues_cr_bl}},
    {{IODirection_ar_cl, IOValues_ar_cl},
     {IODirection_ar_piste, IOValues_ar_piste},
     {IODirection_ar_bl, IOValues_ar_bl}},
    {{IODirection_br_cl, IOValues_br_cl},
     {IODirection_br_piste, IOValues_br_piste},
     {IODirection_br_bl, IOValues_br_bl}},

};
adc1_channel_t analogtestsettings[3] = {cl_analog, piste_analog, bl_analog};
adc1_channel_t analogtestsettings_right[3] = {cr_analog, ar_analog, br_analog};

void testWiresOnByOne() {
    for (int Nr = 0; Nr < 3; Nr++) {
        for (int j = 0; j < 3; j++) {
            Set_IODirectionAndValue(testsettings[Nr][j][0], testsettings[Nr][j][1]);
            measurements[Nr][j] = getDifferentialSample(analogtestsettings_right[Nr], analogtestsettings[j]);
        }
    }
    return;
}

bool WirePluggedIn(int threashold) {
    for (int Nr = 0; Nr < 3; Nr++) {
        for (int j = 0; j < 3; j++) {
            if (measurements[Nr][j] < threashold) {
                return true;
            }
        }
    }
    return false;
}

bool WirePluggedInFoil(int threashold) {
    for (int Nr = 0; Nr < 3; Nr++) {
        for (int j = 0; j < 3; j++) {
            if (!((Nr == 1 && j == 0) || (Nr == 2 && j == 0)))  // Skip the Lamé wire to A or B
            {
                if (measurements[Nr][j] < threashold) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool WirePluggedInEpee(int threashold) {
    for (int Nr = 0; Nr < 3; Nr++) {
        for (int j = 1; j < 3; j++) {
            // if (!((Nr == 1 && j == 0) || (Nr == 0 && j == 0)))  // Skip the Lamé wire to A or C or B (Probe mode)
            {
                if (measurements[Nr][j] < threashold) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Checks straigth connections only. Fills in measurement[i][i]
// Returns true if all connections have a resistance lower than 10 Ohm
bool testStraightOnly(int threashold) {
    bool bOK = true;
    for (int Nr = 0; Nr < 3; Nr++) {
        {
            Set_IODirectionAndValue(testsettings[Nr][Nr][0], testsettings[Nr][Nr][1]);

            // measurements[Nr][Nr] = getSample(analogtestsettings[Nr]);
            measurements[Nr][Nr] = getDifferentialSample(analogtestsettings_right[Nr], analogtestsettings[Nr]);
            if (measurements[Nr][Nr] > threashold)
                bOK = false;
        }
    }

    return bOK;
}

int testArBr() {
    Set_IODirectionAndValue(IODirection_ar_br, IOValues_ar_br);
    return (getDifferentialSample(ar_analog, br_analog));
}
int testArCr() {
    Set_IODirectionAndValue(IODirection_ar_cr, IOValues_ar_cr);
    return (getDifferentialSample(ar_analog, cr_analog));
}
int testArCl() {
    Set_IODirectionAndValue(IODirection_ar_cl, IOValues_ar_cl);
    return (getDifferentialSample(ar_analog, cl_analog));
}

int testBrCr() {
    Set_IODirectionAndValue(IODirection_br_cr, IOValues_br_cr);
    return (getDifferentialSample(br_analog, cr_analog));
}

int testBrCl() {
    Set_IODirectionAndValue(IODirection_br_cl, IOValues_br_cl);
    return (getDifferentialSample(br_analog, cl_analog));
}

// Simply broken: no contact between i-i' and no contact with other wires
bool IsBroken(int Nr, int threashold) {
    if ((measurements[Nr][Nr] < threashold))
        return false;
    for (int i = 0; i < 3; i++) {
        if (i != Nr) {
            if (measurements[Nr][i] < threashold)
                return false;
        }
    }
    return true;
}

bool IsSwappedWith(int i, int j, int threashold) {
    if ((measurements[i][j] < threashold) && (measurements[j][i] < threashold))
        return true;
    return false;
}

void init_AD() {
    gpio_set_drive_capability(GPIO_NUM_33, GPIO_DRIVE_CAP_3);  // 40 mA
    gpio_set_drive_capability(GPIO_NUM_21, GPIO_DRIVE_CAP_3);  // 40 mA
    gpio_set_drive_capability(GPIO_NUM_23, GPIO_DRIVE_CAP_3);  // 40 mA
    gpio_set_drive_capability(GPIO_NUM_25, GPIO_DRIVE_CAP_3);  // 40 mA
    gpio_set_drive_capability(GPIO_NUM_5, GPIO_DRIVE_CAP_3);   // 40 mA
    gpio_set_drive_capability(GPIO_NUM_18, GPIO_DRIVE_CAP_3);  // 40 mA
    gpio_set_drive_capability(GPIO_NUM_19, GPIO_DRIVE_CAP_3);  // 40 mA

    Set_IODirectionAndValue(IODirection_ar_bl, IOValues_ar_bl);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    // Warm up ADC with some dummy readings
    int test = adc1_get_raw(ADC1_CHANNEL_3);
    test = adc1_get_raw(ADC1_CHANNEL_4);
    test = adc1_get_raw(ADC1_CHANNEL_5);
    test = adc1_get_raw(ADC1_CHANNEL_6);
    test = adc1_get_raw(ADC1_CHANNEL_7);
    test = adc1_get_raw(ADC1_CHANNEL_0);

    // Remove this line:
    // calibrateADCOffsets();
}

// Remove these global arrays entirely:
// int adc_offset_calibration_high[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// int adc_offset_calibration_low[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Remove the entire calibrateADCOffsets() function (100+ lines)

// Simplify getCalibratedVoltage to just return uncalibrated voltage:
int getCalibratedVoltage(int raw_value, adc1_channel_t channel) {
    return esp_adc_cal_raw_to_voltage(raw_value, &adc_chars);
}