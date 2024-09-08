import matplotlib.pyplot as plt
import numpy as np

n_values = []
average_times = []


with open("avg_time_per_client.txt") as f:
    for line in f:
        n, avg_time = map(float, line.split(" "))
        n_values.append(n)
        average_times.append(avg_time)
       

n_values = np.array(n_values)
average_times = np.array(average_times)
# coefficients = np.polyfit(n_values, average_times, 5)
# polynomial = np.poly1d(coefficients)
# y_curve=polynomial(n_values)

plt.figure(figsize=(10, 6))
plt.errorbar(n_values, average_times, fmt='o', linestyle='none', capsize=5, label="Average Completion Time")
plt.plot(n_values, average_times, color='red')

plt.xlabel('n values')
plt.ylabel('Completion Time (seconds)')
plt.title('Average Completion Time vs n Values')

plt.grid(True)
# plt.legend()

plt.savefig("plot.png")
