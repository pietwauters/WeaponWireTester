#pragma once

#include "driver/adc.h"
#include "esp_adc_cal.h"

constexpr int CurrentVersion = 3;

// Empirical resistor calibrator using your proven model:
// V_diff = V_gpio * R / (R + R1_R2 + Correction/R)
class EmpiricalResistorCalibrator {
   public:
    // Initialize with ADC channel for differential measurement
    bool begin(adc1_channel_t adc_channel_top, adc1_channel_t adc_channel_bottom);

    // Interactive calibration using multiple known resistors (least squares)
    bool calibrate_interactively_empirical();

    // Get resistance from differential measurement using empirical model
    float get_resistance_empirical(float v_diff_measured);

    // Get ADC raw threshold for a resistance threshold, compensating for test lead resistance
    uint32_t get_adc_threshold_for_resistance_with_leads(float resistance_threshold, float lead_resistance = 0.0f);

    // Save/load calibration
    bool save_calibration_to_nvs(const char* nvs_namespace = "emp_cal");
    bool load_calibration_from_nvs(const char* nvs_namespace = "emp_cal");

    // Measurement functions
    struct EmpiricalReading {
        float v_top;
        float v_bottom;
        float v_diff;
        float resistance;
    };

    EmpiricalReading read_differential_empirical(int samples = 100);

    // Getters for calibration parameters
    float get_v_gpio() const { return v_gpio; }
    float get_r1_r2() const { return r1_r2; }
    float get_correction() const { return correction; }

    // Check if calibrator is properly calibrated
    bool is_calibrated() const { return v_gpio > 0 && r1_r2 > 0; }

   private:
    // ADC channels
    adc1_channel_t channel_top;
    adc1_channel_t channel_bottom;

    // Empirical model parameters: V_diff = V_gpio * R / (R + R1_R2 + Correction/R)
    float v_gpio = 0.0f;      // Effective GPIO voltage
    float r1_r2 = 0.0f;       // Combined fixed resistance
    float correction = 0.0f;  // Current-dependent correction factor

    // ADC calibration
    esp_adc_cal_characteristics_t adc_chars;

    // Helper functions
    float calculate_model_voltage(float R_known, float v_gpio, float r1_r2, float correction);
    float voltage_to_resistance(float v_diff, float v_gpio, float r1_r2, float correction);
    bool least_squares_fit(float* R_values, float* V_diff_values, int num_points);
    int voltage_to_adc_raw(float voltage);  // Convert voltage to ADC raw value
    void wait_for_enter();
    float read_float_from_uart();  // ESP32-safe float input with WDT reset
    char read_char_from_uart();    // ESP32-safe char input with WDT reset

    // Multi-stage calibration helper functions
    float optimize_slope_weighted(float* R_values, float* V_diff_values, int num_points, float v_gpio_open);
    float optimize_correction_sweep(float* R_values, float* V_diff_values, int num_points, float v_gpio_open,
                                    float r1_r2_fixed);
    void show_calibration_quality(float* R_values, float* V_diff_values, int num_points);
    void interactive_parameter_tuning(float* R_values, float* V_diff_values, int num_points);
};

class DifferentialResistorCalibrator {
   public:
    // Initialize with top and bottom ADC channels
    bool begin(adc1_channel_t adc_channel_top, adc1_channel_t adc_channel_bottom);

    // Interactive calibration using known resistor
    bool calibrate_interactively(float R_known);

    // Get resistance from differential measurement
    float get_resistance_from_differential(float v_top, float v_bottom);

    // Get ADC threshold for a given resistance (differential mode)
    int get_adc_threshold_for_resistance(float R);

    // Save/load calibration
    bool save_calibration_to_nvs(const char* nvs_namespace = "diff_cal");
    bool load_calibration_from_nvs(const char* nvs_namespace = "diff_cal");

    // Measurement functions
    struct DifferentialReading {
        float v_top;
        float v_bottom;
        float v_diff;
        float resistance;
    };

    DifferentialReading read_differential(int samples = 8);
    DifferentialReading read_differential_average(int samples = 100);
    DifferentialReading read_differential_median(int samples = 100);
    DifferentialReading read_differential_trimmed_mean(int samples = 100, float trim_percent = 0.2f);

    // Getters
    float get_vcc() const { return v_gpio; }
    float get_r1_eff() const { return r1_eff; }
    float get_r3_eff() const { return r3_eff; }

   private:
    // ADC channels
    adc1_channel_t channel_top;
    adc1_channel_t channel_bottom;

    // Calibration parameters
    float v_gpio = 0.0f;  // Supply voltage
    float r1_eff = 0.0f;  // Effective R1 (47Ω + GPIO Ron + parasitic resistances)
    float r3_eff = 0.0f;  // Effective R3 (47Ω + GPIO Ron + parasitic resistances)
    int Version = CurrentVersion;
    // ADC calibration
    esp_adc_cal_characteristics_t adc_chars;

    // Helper functions
    float read_voltage_single(adc1_channel_t channel);
    float read_voltage_average(adc1_channel_t channel, int samples);
    int voltage_to_adc_raw(float voltage);
    void wait_for_enter();
    char wait_for_key();

    // Fast ADC read (you can replace with your FastADC1 if needed)
    inline uint32_t fast_adc1_get_raw_inline(adc1_channel_t channel) { return adc1_get_raw(channel); }
};