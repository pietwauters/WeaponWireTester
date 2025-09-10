import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

# Your measured data (Rx in ohms, Vx in volts)
data = np.array([
    (0.33,0.001),
    (0.56,0.005),
    (0.82,0.013),
    (1,0.018),
    (1.2,0.024),
    (1.5,0.028),
    (1.8,0.032),
    (2.0,0.043),
    (2.2,0.045),
    (2.5,0.047),
    (2.7,0.055),
    (3.2,0.071),
    (3.7,0.081),
    (4.7,0.105),
    (5.7,0.132),
    (6.9,0.160),
    (7.9,0.187),
    (8.2,0.187),
    (9.2,0.217),
    (10.2,0.240),
    (11.4,0.256),
    (12.9,0.290),
    (13.9,0.315),
    (16.1,0.360)

    
    
])

rx_data = data[:, 0]
vx_data = data[:, 1]

# Model: Vx = 3.3 * Rx / (Rx + A)
def model(rx, A):
    return 3.3 * rx / (rx + A)

# Fit the model to the data
popt, pcov = curve_fit(model, rx_data, vx_data)
A_fit = popt[0]
ron_total = A_fit - 94

print(f"Calibrated A = {A_fit:.2f} Ω")
print(f"Estimated Ron_H + Ron_L = {ron_total:.2f} Ω")

# Inverse model to estimate Rx from Vx
def estimate_rx(vx, A=A_fit):
    return (vx * A) / (3.3 - vx)

# Example usage
vx_measured = 0.088  # change this to your measured Vx
estimated_rx = estimate_rx(vx_measured)
print(f"Estimated Rx for Vx = {vx_measured:.3f} V: Rx = {estimated_rx:.2f} Ω")

# Optional: Plot to visualize the fit
rx_plot = np.linspace(1, 20, 200)
vx_fit = model(rx_plot, A_fit)

plt.scatter(rx_data, vx_data, label="Measured", color="red")
plt.plot(rx_plot, vx_fit, label=f"Fitted Model (A={A_fit:.2f})", color="blue")
plt.xlabel("Rx (Ω)")
plt.ylabel("Vx (V)")
plt.title("Model Fit: Vx = 3.3 * Rx / (Rx + A)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
