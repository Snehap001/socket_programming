import matplotlib.pyplot as plt
import numpy as np

n_values= []
avg_times_fifo = []

avg_times_rr = []

def read_file(filename):
    data=[]
    with open(filename,'r') as file:
        for line in file:
            n,avg=line.split()
            data.append(((int(n)),float(avg)))
    return data
def write_combined(file1,file2,output):
    with open(output,'w') as outfile:
        for (n1, avg1), (_, avg2)in zip(file1, file2):
            outfile.write(f"{n1} {avg1} {avg2}\n")
file1='avg_time_fifo.txt'
file2='avg_time_rr.txt'
f1_data=read_file(file1)
f2_data=read_file(file2)
write_combined(f1_data,f2_data,'avg_time.txt')

with open("avg_time.txt") as f:
    for line in f:
        n,fifo,rr = map(float, line.split(" "))
        n_values.append(n)
        avg_times_fifo.append(fifo)
        avg_times_rr.append(rr)


n_values = np.array(n_values)
avg_times_fifo = np.array(avg_times_fifo)
avg_times_rr = np.array(avg_times_rr)

# coefficients = np.polyfit(n_values, average_times, 5)
# polynomial = np.poly1d(coefficients)
# y_curve=polynomial(n_values)

plt.figure(figsize=(10, 6))
plt.plot(n_values, avg_times_fifo, color='red',label="FIFO ")
plt.plot(n_values, avg_times_rr, color='blue',label="RR")

plt.xlabel('n values')
plt.ylabel('Completion Time (seconds)')
plt.title('Average Completion Time vs n Values')

plt.grid(True)
plt.legend()

plt.savefig("plot.png")
