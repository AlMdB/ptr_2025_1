import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import re

def parse_performance_file(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    sections = re.split(r'(Sem Carga|Com Carga)', content)
    
    sem_carga_data = []
    com_carga_data = []
    
    for i in range(len(sections)):
        if sections[i] == 'Sem Carga' and i+1 < len(sections):
            lines = sections[i+1].strip().split('\n')
            for line in lines:
                if line and not line.startswith('Thread') and not line.startswith('-'):
                    parts = line.split()
                    if len(parts) >= 7:
                        sem_carga_data.append({
                            'Thread': parts[0],
                            'Media': float(parts[1]),
                            'Desvio': float(parts[2]),
                            'Max': float(parts[3]),
                            'Min': float(parts[4]),
                            'Jitter': float(parts[5]),
                            'Misses': int(parts[6])
                        })
        
        elif sections[i] == 'Com Carga' and i+1 < len(sections):
            lines = sections[i+1].strip().split('\n')
            for line in lines:
                if line and not line.startswith('Thread') and not line.startswith('-'):
                    parts = line.split()
                    if len(parts) >= 7:
                        com_carga_data.append({
                            'Thread': parts[0],
                            'Media': float(parts[1]),
                            'Desvio': float(parts[2]),
                            'Max': float(parts[3]),
                            'Min': float(parts[4]),
                            'Jitter': float(parts[5]),
                            'Misses': int(parts[6])
                        })
    
    return pd.DataFrame(sem_carga_data), pd.DataFrame(com_carga_data)

plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 10
plt.rcParams['axes.titlesize'] = 12
plt.rcParams['legend.fontsize'] = 9

df_without, df_with = parse_performance_file('performance_metrics.txt')

fig, axes = plt.subplots(2, 2, figsize=(15, 12))
fig.suptitle('Sistema de Controle Robótico: Desempenho Com vs Sem Carga', fontsize=16)

x = np.arange(len(df_without['Thread']))
width = 0.35

axes[0,0].bar(x - width/2, df_without['Jitter']*1000, width, label='Sem Carga', color='#3B82F6')
axes[0,0].bar(x + width/2, df_with['Jitter']*1000, width, label='Com Carga', color='#EF4444')
axes[0,0].set_title('Comparação de Jitter (ms)')
axes[0,0].set_ylabel('Jitter (ms)')
axes[0,0].set_xticks(x)
axes[0,0].set_xticklabels(df_without['Thread'], rotation=45)
axes[0,0].legend()
axes[0,0].grid(True, alpha=0.3)

axes[0,1].bar(x - width/2, df_without['Desvio']*1000, width, label='Sem Carga', color='#10B981')
axes[0,1].bar(x + width/2, df_with['Desvio']*1000, width, label='Com Carga', color='#F59E0B')
axes[0,1].set_title('Comparação de Desvio Padrão (ms)')
axes[0,1].set_ylabel('Desvio Padrão (ms)')
axes[0,1].set_xticks(x)
axes[0,1].set_xticklabels(df_without['Thread'], rotation=45)
axes[0,1].legend()
axes[0,1].grid(True, alpha=0.3)

axes[1,0].bar(x - width/2, df_without['Misses'], width, label='Sem Carga', color='#8B5CF6')
axes[1,0].bar(x + width/2, df_with['Misses'], width, label='Com Carga', color='#EC4899')
axes[1,0].set_title('Comparação de Perdas de Thread')
axes[1,0].set_ylabel('Perdas')
axes[1,0].set_xticks(x)
axes[1,0].set_xticklabels(df_without['Thread'], rotation=45)
axes[1,0].legend()
axes[1,0].grid(True, alpha=0.3)

jitter_change = ((df_with['Jitter'] - df_without['Jitter']) / df_without['Jitter'] * 100)
colors = ['green' if x < 0 else 'red' for x in jitter_change]
axes[1,1].bar(x, jitter_change, color=colors, alpha=0.7)
axes[1,1].set_title('Mudança no Jitter (%)')
axes[1,1].set_ylabel('Mudança (%)')
axes[1,1].set_xticks(x)
axes[1,1].set_xticklabels(df_without['Thread'], rotation=45)
axes[1,1].axhline(y=0, color='black', linestyle='-', alpha=0.3)
axes[1,1].grid(True, alpha=0.3)

plt.tight_layout()
fig.savefig('robot_performance_comparison.png') 
plt.show()

print("Resumo da Análise de Desempenho:")
print("="*50)
for i, thread in enumerate(df_without['Thread']):
    jitter_diff = (df_with['Jitter'][i] - df_without['Jitter'][i]) / df_without['Jitter'][i] * 100
    miss_diff = df_with['Misses'][i] - df_without['Misses'][i]
    print(f"{thread:12} | Jitter: {jitter_diff:+6.1f}% | Perdas: {miss_diff:+3d}")

print("\nInsights Principais:")
print("- Contraintuitivamente, a carga MELHOROU o jitter da maioria das threads")
print("- Comportamento típico de SO não-RT: melhor desempenho médio sob carga")
print("- Provável causa: escalonamento de frequência da CPU e efeitos de cache")
print("- Trade-off: melhor desempenho vs. menor previsibilidade")

improvement_threads = [df_without['Thread'][i] for i, x in enumerate(jitter_change) if x < -10]
if improvement_threads:
    print(f"- Melhorias significativas: {', '.join(improvement_threads)}")

plt.figure(figsize=(12, 8))
thread_names = df_without['Thread']
jitter_without = df_without['Jitter'] * 1000
jitter_with = df_with['Jitter'] * 1000

plt.scatter(jitter_without, jitter_with, s=100, alpha=0.7, c=['red' if x > y else 'green' for x, y in zip(jitter_with, jitter_without)])
plt.plot([0, max(max(jitter_without), max(jitter_with))], [0, max(max(jitter_without), max(jitter_with))], 'k--', alpha=0.5)

for i, thread in enumerate(thread_names):
    plt.annotate(thread, (jitter_without[i], jitter_with[i]), xytext=(5, 5), textcoords='offset points', fontsize=9)

plt.xlabel('Jitter Sem Carga (ms)')
plt.ylabel('Jitter Com Carga (ms)')
plt.title('Jitter: Com vs Sem Carga\n(Pontos abaixo da diagonal = melhoria com carga)')
plt.grid(True, alpha=0.3)
plt.savefig('jitter_scatter_plot.png')
plt.show()

df_comparison = pd.DataFrame({
    'Thread': df_without['Thread'],
    'Mudança_Jitter_%': jitter_change,
    'Mudança_Perdas': df_with['Misses'] - df_without['Misses'],
    'Mudança_DesvPad_%': ((df_with['Desvio'] - df_without['Desvio']) / df_without['Desvio'] * 100)
})

print("\nTabela de Comparação Detalhada:")
print(df_comparison.to_string(index=False, float_format='%.1f'))