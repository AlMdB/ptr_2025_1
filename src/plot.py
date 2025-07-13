import pandas as pandas
import matplotlib.pyplot as pyplot

no_rt = pd.read_csv('standard_metrics.txt', sep='\s+', skiprows=1)
rt = pd.read_csv('realtime_metrics.txt', sep='\s+', skiprows=1)

# Plot jitter comparison
plt.figure(figsize=(10, 6))
plt.bar(no_rt['Thread'], no_rt['Jitter(s)'], alpha=0.4, label='Generic Kernel')
plt.bar(rt['Thread'], rt['Jitter(s)'], alpha=0.4, label='Real Time Kernel')
plt.xlabel('Thread')
plt.ylabel('Jitter (s)')
plt.title('Comparacao: Generic Kernel X Real Time Kernel')
plt.legend()
plt.show()