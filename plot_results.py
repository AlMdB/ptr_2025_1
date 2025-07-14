import numpy as np
import matplotlib.pyplot as plt

def read_simulation_data(filename):
    data_lines = []
    with open(filename, 'r') as f:
        next(f)
        for line in f:
            line = line.strip()
            if line:
                values = [x.strip() for x in line.split(',') if x.strip()]           
                if len(values) == 8:
                    try:
                        data_lines.append([float(v) for v in values])
                    except:
                        pass
    return np.array(data_lines)

def calculate_velocities(time, xf, yf, theta):
    dt = np.diff(time)
    dx = np.diff(xf)
    dy = np.diff(yf)
    v = np.sqrt(dx**2 + dy**2) / dt
    dtheta = np.diff(theta)
    dtheta = np.where(dtheta > np.pi, dtheta - 2*np.pi, dtheta)
    dtheta = np.where(dtheta < -np.pi, dtheta + 2*np.pi, dtheta)
    omega = dtheta / dt
    v = np.concatenate([[v[0]], v])
    omega = np.concatenate([[omega[0]], omega])
    return v, omega

def plot_robot_trajectory(data, title_suffix):
    time = data[:, 0]
    x_ref = data[:, 1]
    y_ref = data[:, 2]
    xf = data[:, 5] 
    yf = data[:, 6]
    theta = data[:, 7]
    
    v, omega = calculate_velocities(time, xf, yf, theta)
    error_x = xf - x_ref
    error_y = yf - y_ref
    position_error = np.sqrt(error_x**2 + error_y**2)
    
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle(f'Análise da Trajetória do Robô - {title_suffix}', fontsize=16)
    
    ax1.plot(time, v, 'b-', linewidth=2, label='v (m/s)')
    ax1.plot(time, omega, 'r-', linewidth=2, label='ω (rad/s)')
    ax1.set_xlabel('Tempo (s)')
    ax1.set_ylabel('Velocidades Linear e Angular')
    ax1.set_title('Sinais de Controle Calculados')
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    
    ax2.plot(time, x_ref, 'g--', linewidth=2, label='x_ref')
    ax2.plot(time, y_ref, 'b--', linewidth=2, label='y_ref')
    ax2.plot(time, xf, 'g-', linewidth=2, label='x_real')
    ax2.plot(time, yf, 'b-', linewidth=2, label='y_real')
    ax2.set_xlabel('Tempo (s)')
    ax2.set_ylabel('Posição (m)')
    ax2.set_title('Rastreamento de Posição')
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    ax3.plot(x_ref, y_ref, 'k--', linewidth=2, label='Trajetória de referência', alpha=0.7)
    ax3.plot(xf, yf, 'r-', linewidth=2, label='Trajetória real')
    ax3.plot(xf[0], yf[0], 'go', markersize=8, label='Início')
    ax3.plot(xf[-1], yf[-1], 'rs', markersize=8, label='Fim')
    ax3.set_xlabel('x (m)')
    ax3.set_ylabel('y (m)')
    ax3.set_title('Trajetória do Robô')
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    ax3.axis('equal')
    
    ax4_twin = ax4.twinx()
    line1 = ax4.plot(time, theta, 'r-', linewidth=2, label='θ (rad)')
    ax4.set_xlabel('Tempo (s)')
    ax4.set_ylabel('Orientação (rad)', color='r')
    ax4.tick_params(axis='y', labelcolor='r')
    
    line2 = ax4_twin.plot(time, position_error, 'b-', linewidth=2, label='Erro de posição (m)')
    ax4_twin.set_ylabel('Erro de Posição (m)', color='b')
    ax4_twin.tick_params(axis='y', labelcolor='b')
    ax4.set_title('Orientação e Erro de Rastreamento')
    ax4.grid(True, alpha=0.3)
    
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax4.legend(lines, labels, loc='upper right')
    
    plt.tight_layout()
    
    output_filename = f'robot_trajectory_{title_suffix.lower().replace(" ", "_")}.png'
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    
    print(f"\nEstatísticas da Trajetória - {title_suffix}:")
    print(f"Tempo total: {time[-1]:.2f} s")
    print(f"Posição final: ({xf[-1]:.3f}, {yf[-1]:.3f}) m")
    print(f"Orientação final: {theta[-1]*180/np.pi:.1f} graus")
    print(f"Erro máximo de posição: {np.max(position_error):.3f} m")
    print(f"Erro RMS de posição: {np.sqrt(np.mean(position_error**2)):.3f} m")
    print(f"Gráfico salvo como '{output_filename}'")
    
    plt.show()

def main():
    files = ['output_no_load.txt', 'output_loaded.txt']
    titles = ['Sem Carga', 'Com Carga']
    
    for filename, title in zip(files, titles):
        try:
            data = read_simulation_data(filename)
            if len(data) > 0:
                plot_robot_trajectory(data, title)
        except:
            print(f"Erro ao processar {filename}")

if __name__ == "__main__":
    main()