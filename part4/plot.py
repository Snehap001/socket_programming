import matplotlib.pyplot as plt
import numpy as np

n_values_fifo = []
average_times_fifo = []
n_values_rr = []
average_times_rr = []

with open("avg_time_per_client_fifo.txt") as f:
    for line in f:
        n, avg_time = map(float, line.split(" "))
        n_values_fifo.append(n)
        average_times_fifo.append(avg_time)
with open("avg_time_per_client_rr.txt") as f:
    for line in f:
        n, avg_time = map(float, line.split(" "))
        n_values_rr.append(n)
        average_times_rr.append(avg_time)
       

n_values_fifo = np.array(n_values_fifo)
average_times_fifo = np.array(average_times_fifo)
n_values_rr = np.array(n_values_rr)
average_times_rr = np.array(average_times_rr)
# coefficients = np.polyfit(n_values, average_times, 5)
# polynomial = np.poly1d(coefficients)
# y_curve=polynomial(n_values)

plt.figure(figsize=(10, 6))
plt.errorbar(n_values_fifo, average_times_fifo, fmt='o', linestyle='none', capsize=5, label="Fifo")
plt.plot(n_values_fifo, average_times_fifo, color='red')
plt.errorbar(n_values_rr, average_times_rr, fmt='o', linestyle='none', capsize=5, label="Round Robin")
plt.plot(n_values_rr, average_times_rr, color='blue')

plt.xlabel('n values')
plt.ylabel('Completion Time (seconds)')
plt.title('Average Completion Time vs n Values')

plt.grid(True)
plt.legend()

plt.savefig("plot.png")
