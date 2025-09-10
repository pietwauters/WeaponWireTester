/**
 * Numerical Analysis of Wire Resistance Compensation
 *
 * Your empirical model: V_diff = V_gpio * R / (R + R1_R2 + Correction/R)
 *
 * Question: How does adding wire resistance affect the voltage?
 * R_total = R_target + R_wire
 * V_with_wire = V_gpio * R_total / (R_total + R1_R2 + Correction/R_total)
 */

#include <math.h>
#include <stdio.h>

// Function implementing your empirical model
float calculate_voltage(float R, float V_gpio, float R1_R2, float correction) {
    if (R <= 0)
        return 0.0f;
    return V_gpio * R / (R + R1_R2 + correction / R);
}

// Analysis function
void analyze_wire_compensation() {
    // Use realistic calibrated values (you'll need to substitute your actual values)
    float V_gpio = 3.3f;      // Typical GPIO voltage
    float R1_R2 = 94.0f;      // Your recent calibration result
    float correction = 6.0f;  // Your manually tuned value

    printf("=== NUMERICAL ANALYSIS: Wire Resistance Compensation ===\n");
    printf("Empirical model: V_diff = %.2fV * R / (R + %.1fΩ + %.1f/R)\n\n", V_gpio, R1_R2, correction);

    // Test range: 0-10Ω resistors with 0-1Ω wire resistance
    printf("Analysis for 0-10Ω range with 0-1Ω wire resistance:\n");
    printf("R_target  V_no_wire  V_0.5Ω_wire  V_1.0Ω_wire  Drop_0.5Ω  Drop_1.0Ω  Error_0.5%%  Error_1.0%%\n");
    printf("------------------------------------------------------------------------------------\n");

    float resistors[] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
    int num_resistors = sizeof(resistors) / sizeof(resistors[0]);

    for (int i = 0; i < num_resistors; i++) {
        float R_target = resistors[i];

        // Voltage with perfect connection (no wire resistance)
        float V_clean = calculate_voltage(R_target, V_gpio, R1_R2, correction);

        // Voltage with 0.5Ω wire resistance
        float V_05_wire = calculate_voltage(R_target + 0.5f, V_gpio, R1_R2, correction);

        // Voltage with 1.0Ω wire resistance
        float V_10_wire = calculate_voltage(R_target + 1.0f, V_gpio, R1_R2, correction);

        // Voltage drops (should be positive - voltage decreases with wire resistance)
        float drop_05 = V_clean - V_05_wire;
        float drop_10 = V_clean - V_10_wire;

        // Percentage errors (should be positive)
        float error_05 = (drop_05 / V_clean) * 100.0f;
        float error_10 = (drop_10 / V_clean) * 100.0f;

        printf("%6.1fΩ   %8.1fmV   %9.1fmV   %9.1fmV   %7.1fmV   %7.1fmV   %6.1f%%   %6.1f%%\n", R_target,
               V_clean * 1000, V_05_wire * 1000, V_10_wire * 1000, drop_05 * 1000, drop_10 * 1000, error_05, error_10);
    }

    printf("\n=== COMPENSATION ANALYSIS ===\n");

    // Check if voltage drop is approximately linear with wire resistance
    printf("Linearity check (voltage drop per Ω of wire):\n");
    printf("R_target  Drop/Ω(0.5Ω)  Drop/Ω(1.0Ω)  Linearity_Check\n");
    printf("--------------------------------------------------------\n");

    bool is_linear = true;
    float total_drop_per_ohm = 0.0f;

    for (int i = 0; i < num_resistors; i++) {
        float R_target = resistors[i];
        float V_clean = calculate_voltage(R_target, V_gpio, R1_R2, correction);
        float V_05_wire = calculate_voltage(R_target + 0.5f, V_gpio, R1_R2, correction);
        float V_10_wire = calculate_voltage(R_target + 1.0f, V_gpio, R1_R2, correction);

        float drop_per_ohm_05 = (V_clean - V_05_wire) / 0.5f;
        float drop_per_ohm_10 = (V_clean - V_10_wire) / 1.0f;

        float linearity_error = fabs(drop_per_ohm_05 - drop_per_ohm_10) / drop_per_ohm_10 * 100.0f;

        printf("%6.1fΩ    %8.2fmV     %8.2fmV      %5.1f%%\n", R_target, drop_per_ohm_05 * 1000, drop_per_ohm_10 * 1000,
               linearity_error);

        total_drop_per_ohm += drop_per_ohm_10;

        if (linearity_error > 10.0f) {
            is_linear = false;
        }
    }

    float avg_drop_per_ohm = total_drop_per_ohm / num_resistors;

    printf("\n=== CONCLUSIONS ===\n");
    printf("Average voltage drop per Ω of wire: %.2fmV/Ω\n", avg_drop_per_ohm * 1000);
    printf("Linearity: %s (errors < 10%%)\n", is_linear ? "GOOD" : "POOR");

    if (is_linear) {
        printf("\n✓ SIMPLE COMPENSATION WORKS!\n");
        printf("Strategy: Measure wire voltage drop once, subtract from all thresholds\n");
        printf("Formula: V_compensated = V_original - (%.3f * R_wire)\n", avg_drop_per_ohm);

        // Convert to ADC terms (assuming 12-bit ADC, 3.3V range)
        float adc_counts_per_mv = 4096.0f / 3300.0f;
        printf("In ADC counts: ADC_compensated = ADC_original - (%.1f * R_wire_ohms)\n",
               avg_drop_per_ohm * 1000 * adc_counts_per_mv);
    } else {
        printf("\n✗ COMPLEX COMPENSATION NEEDED!\n");
        printf("Non-linear relationship requires resistance-specific correction\n");
    }

    printf("\n=== PRACTICAL RECOMMENDATION ===\n");
    printf("1. Measure your test wire resistance: R_wire\n");
    printf("2. Calculate voltage drop: V_drop = %.3f * R_wire\n", avg_drop_per_ohm);
    printf("3. Use: get_compensated_adc_threshold(original_threshold, V_drop)\n");
}

int main() {
    analyze_wire_compensation();
    return 0;
}
