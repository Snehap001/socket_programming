import matplotlib.pyplot as plt
import numpy as np
import scipy.stats as stats

def read_times(file_path):
    """Reads completion times from a file."""
    with open(file_path, 'r') as file:
        times = [float(line.strip()) for line in file]
    return times

def plot_times(times, p_values):
    """Plots the average completion time with confidence intervals."""
    mean_times = np.array(times)
    
    # Calculate confidence intervals
    confidence = 0.95
    n = len(mean_times)
    stderr = stats.sem(mean_times)
    confidence_interval = stderr * stats.t.ppf((1 + confidence) / 2., n - 1)

    plt.figure(figsize=(10, 6))
    plt.errorbar(p_values, mean_times, yerr=confidence_interval, fmt='-o', capsize=5, label='Average Completion Time with 95% Confidence Interval')

    plt.xlabel('Number of Words per Packet (p)')
    plt.ylabel('Completion Time (seconds)')
    plt.title('Completion Time vs. Number of Words per Packet')
    plt.grid(True)
    plt.legend()
    plt.savefig('plot.png')
    plt.show()

if __name__ == "__main__":
    # Read times from the file
    file_path = 'times.txt'
    times = read_times(file_path)

    # Define the range of p values (1 to 10)
    p_values = list(range(1, 11))

    # Plot the data
    plot_times(times, p_values)
