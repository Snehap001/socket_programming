import matplotlib.pyplot as plt
import numpy as np

n_values= []
avg_times_slotted = []

avg_times_beb = []
avg_times_sbeb=[]
def read_file(filename):
    data=[]
    with open(filename,'r') as file:
        for line in file:
            n,avg=line.split()
            data.append(((int(n)),float(avg)))
    return data
def write_combined(file1,file2,file3,output):
    with open(output,'w') as outfile:
        for (n1, avg1), (_, avg2), (_, avg3) in zip(file1, file2, file3):
            outfile.write(f"{n1} {avg1} {avg2} {avg3}\n")
file1='avg_time_aloha.txt'
file2='avg_time_beb.txt'
file3='avg_time_cscd.txt'
f1_data=read_file(file1)
f2_data=read_file(file2)
f3_data=read_file(file3)
write_combined(f1_data,f2_data,f3_data,'avg_time.txt')
with open("avg_time.txt") as f:
    for line in f:
        n, slotted,beb,sbeb = map(float, line.split(" "))
        n_values.append(n)
        avg_times_slotted.append(slotted)
        avg_times_beb.append(beb)
        avg_times_sbeb.append(sbeb)


n_values = np.array(n_values)
avg_times_slotted = np.array(avg_times_slotted)
avg_times_beb = np.array(avg_times_beb)
avg_times_sbeb = np.array(avg_times_sbeb)

# coefficients = np.polyfit(n_values, average_times, 5)
# polynomial = np.poly1d(coefficients)
# y_curve=polynomial(n_values)

plt.figure(figsize=(10, 6))
plt.plot(n_values, avg_times_slotted, color='red',label="Slotted Aloha")
plt.plot(n_values, avg_times_beb, color='blue',label="BEB")
plt.plot(n_values, avg_times_sbeb, color='green',label="Sensing BEB")

plt.xlabel('n values')
plt.ylabel('Completion Time (seconds)')
plt.title('Average Completion Time vs n Values')

plt.grid(True)
plt.legend()

plt.savefig("plot.png")
