/**
 * Proper Analysis: Wire Resistance Compensation Error
 *
 * Your empirical model: V_diff = V_gpio * R / (R + R1_R2 + Correction/R)
 *
 * Questions:
 * 1. How much does voltage change when we add wire resistance?
 * 2. Can we approximate this by simple voltage addition?
 * 3. What's the error if we use simple addition vs. exact calculation?
 */

#include <math.h>
#include <stdio.h>

// Your empirical model
float calculate_voltage(float R, float V_gpio, float R1_R2, float correction) {
    if (R <= 0)
        return 0.0f;
    return V_gpio * R / (R + R1_R2 + correction / R);
}

void analyze_compensation_error() {
    // Use your calibrated values
    float V_gpio = 3.3f;
    float R1_R2 = 94.0f;
    float correction = 6.0f;

    printf("=== WIRE COMPENSATION ERROR ANALYSIS ===\n");
    printf("Model: V_diff = %.2fV * R / (R + %.1fΩ + %.1f/R)\n\n", V_gpio, R1_R2, correction);

    // Test different wire resistances
    float wire_resistances[] = {0.2f, 0.5f, 1.0f};
    int num_wires = sizeof(wire_resistances) / sizeof(wire_resistances[0]);

    // Test different target resistances in your range
    float target_resistances[] = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
    int num_targets = sizeof(target_resistances) / sizeof(target_resistances[0]);

    for (int w = 0; w < num_wires; w++) {
        float R_wire = wire_resistances[w];

        printf("=== Wire Resistance: %.1fΩ ===\n", R_wire);
        printf("R_target  V_clean   V_with_wire  Wire_V_drop  Simple_Est  Exact_Error  Simple_Error\n");
        printf("--------------------------------------------------------------------------------\n");

        for (int i = 0; i < num_targets; i++) {
            float R_target = target_resistances[i];

            // Voltage with clean connection (no wire)
            float V_clean = calculate_voltage(R_target, V_gpio, R1_R2, correction);

            // Voltage with wire resistance (exact)
            float V_with_wire = calculate_voltage(R_target + R_wire, V_gpio, R1_R2, correction);

            // Actual voltage increase due to wire
            float wire_voltage_increase = V_with_wire - V_clean;

            // What would simple voltage addition predict?
            // If we measure wire separately: V_wire_alone = V_gpio * R_wire / (R_wire + R1_R2 + correction/R_wire)
            float V_wire_alone = calculate_voltage(R_wire, V_gpio, R1_R2, correction);
            float simple_estimate = V_clean + V_wire_alone;

            // Errors
            float exact_error_mv = wire_voltage_increase * 1000;
            float simple_error_mv = (simple_estimate - V_with_wire) * 1000;
            float simple_error_percent = fabs(simple_error_mv / (V_with_wire * 1000)) * 100;

            printf("%6.1fΩ   %7.1fmV   %9.1fmV    %8.1fmV   %8.1fmV    %8.1fmV    %8.1f%%\n", R_target, V_clean * 1000,
                   V_with_wire * 1000, exact_error_mv, simple_estimate * 1000, simple_error_mv, simple_error_percent);
        }
        printf("\n");
    }

    printf("=== ANALYSIS CONCLUSIONS ===\n");

    // Test the linear approximation approach
    printf("\nTesting LINEAR APPROXIMATION method:\n");
    printf("Assumption: ΔV ≈ constant for small wire resistances\n");

    float R_test = 2.0f;  // Use 2Ω as reference
    float small_wire = 0.1f;

    float V_ref = calculate_voltage(R_test, V_gpio, R1_R2, correction);
    float V_ref_wire = calculate_voltage(R_test + small_wire, V_gpio, R1_R2, correction);
    float dV_dR = (V_ref_wire - V_ref) / small_wire;  // Voltage derivative

    printf("Voltage derivative at %.1fΩ: %.2fmV/Ω\n", R_test, dV_dR * 1000);

    printf("\nLinear approximation errors for different wire resistances:\n");
    printf("R_wire   Linear_Est   Exact_Value   Error_mV   Error_%%\n");
    printf("-------------------------------------------------------\n");

    for (int i = 0; i < num_wires; i++) {
        float R_wire = wire_resistances[i];
        float linear_est = V_ref + dV_dR * R_wire;
        float exact_val = calculate_voltage(R_test + R_wire, V_gpio, R1_R2, correction);
        float error_mv = (linear_est - exact_val) * 1000;
        float error_percent = fabs(error_mv / (exact_val * 1000)) * 100;

        printf("%5.1fΩ     %8.1fmV     %8.1fmV    %6.1fmV   %6.1f%%\n", R_wire, linear_est * 1000, exact_val * 1000,
               error_mv, error_percent);
    }

    printf("\n=== RECOMMENDATIONS ===\n");
    printf("1. EXACT METHOD: Use calculate_voltage(R_target + R_wire) for thresholds\n");
    printf("2. LINEAR APPROX: Add %.2fmV per Ω of wire (good for R_wire < 0.5Ω)\n", dV_dR * 1000);
    printf("3. SIMPLE ADDITION: NOT RECOMMENDED (large errors)\n");

    // Calculate ADC count equivalent
    float adc_per_mv = 4096.0f / 3300.0f;  // Approximate
    printf("\nIn ADC counts: Add %.1f counts per Ω of wire resistance\n", dV_dR * 1000 * adc_per_mv);
}

int main() {
    analyze_compensation_error();
    return 0;
}
