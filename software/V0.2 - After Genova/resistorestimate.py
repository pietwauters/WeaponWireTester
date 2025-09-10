import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

# Define the model function
def model(x, Vmax, R):
    return Vmax * x / (R + x)

# Replace with your measurements (xi, V(xi))
x_data = np.array([0.33,
0.56,
0.82,
1.0,
1.2,
1.5,
1.8,
2.0,
2.2,
2.5,
2.7,
3.2,
3.7,
4.7,
5.7,
6.9,
7.9,
8.2,
9.2,
10.2,
11.4,
12.9,
13.9,
16.1])          # Your x values (0–15)
V_data = np.array([0.001,
0.005,
0.013,
0.018,
0.024,
0.028,
0.032,
0.043,
0.045,
0.047,
0.055,
0.071,
0.081,
0.105,
0.132,
0.160,
0.187,
0.187,
0.217,
0.240,
0.256,
0.300,
0.322,
0.368]) # Your V values

# Initial parameter guesses (from your prior knowledge)
# Initial guesses and bounds
initial_guess = [2.992, 130]  # [Vmax, R]
param_bounds = ([2.9, 94], [3.0, 150])  # Force Vmax ∈ [2.5,3.5], R ∈ [50,300]

# Perform nonlinear fit
params_opt, params_cov = curve_fit(
    model, x_data, V_data, 
    p0=initial_guess, 
    bounds=param_bounds  # Critical constraint
)
Vmax_opt, R_opt = params_opt

# Calculate parameter uncertainties
Vmax_err, R_err = np.sqrt(np.diag(params_cov))

# Generate fitted curve
x_fit = np.linspace(0, 15, 100)
V_fit = model(x_fit, Vmax_opt, R_opt)

# Create plot (FIXED TITLE STRING)
plt.figure(figsize=(8, 5))
plt.scatter(x_data, V_data, s=100, label='Data', zorder=3, 
            edgecolor='k', facecolor='#1f77b4')
plt.plot(x_fit, V_fit, 'r-', label=fr'Fit: $V_{{\max}}$ = {Vmax_opt:.3f} $\pm$ {Vmax_err:.3f}' + '\n' +
                                  fr'$R$ = {R_opt:.1f} $\pm$ {R_err:.1f}', 
         linewidth=2)

# Use raw string (r"...") and proper LaTeX escaping
plt.title(r'Nonlinear Fit: $V(x) = \frac{V_{\max} \cdot x}{R + x}$', fontsize=14)  # FIXED LINE
plt.xlabel('x', fontsize=12)
plt.ylabel('V(x)', fontsize=12)
plt.legend(fontsize=12, frameon=True, shadow=True)
plt.grid(True, linestyle='--', alpha=0.7)
plt.tight_layout()
plt.show()

# Print results
print(f"Optimized parameters:")
print(f"V_max = {Vmax_opt:.4f} ± {Vmax_err:.4f}")
print(f"R     = {R_opt:.1f} ± {R_err:.1f}")