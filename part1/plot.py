import matplotlib.pyplot as plt
import numpy as np

p_values = []
average_times = []
confidence_intervals = []

with open("average_times_with_ci.txt") as f:
    for line in f:
        p, avg_time, ci = map(float, line.split(","))
        p_values.append(p)
        average_times.append(avg_time)
        confidence_intervals.append(ci)

p_values = np.array(p_values)
average_times = np.array(average_times)
confidence_intervals = np.array(confidence_intervals)
coefficients = np.polyfit(p_values, average_times, 5)
polynomial = np.poly1d(coefficients)
y_curve=polynomial(p_values)

plt.figure(figsize=(10, 6))
plt.errorbar(p_values, average_times, yerr=confidence_intervals, fmt='o', linestyle='none', capsize=5, label="Average Completion Time")
plt.plot(p_values, y_curve, color='red')

plt.xlabel('p values')
plt.ylabel('Completion Time (seconds)')
plt.title('Average Completion Time vs p Values with 95% Confidence Intervals')

plt.grid(True)
# plt.legend()

plt.savefig("plot.png")
