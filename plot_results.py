import sys
import numpy as np
import matplotlib.pyplot as plt

def read_simulation_data(filename):
    print(f"Arquivo {filename}")   
    data_lines = []
    
    with open(filename, 'r') as f:
        next(f)
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            values = [x.strip() for x in line.split('\t') if x.strip()]           
            
            if len(values) == 6:
                try:
                    float_values = [float(v) for v in values]
                    data_lines.append(float_values)
                except ValueError:
                    print("erro de valor float")
            else:
                print("erro na quant de colunas")
    return np.array(data_lines)

def plot(data):  
        time = data[:, 0]
        v = data[:, 1]
        omega = data[:, 2] 
        xf = data[:, 3]
        yf = data[:, 4]
        theta_f = data[:, 5]
        
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        ax1.plot(time, v, 'b-', linewidth=2, label='v (m/s)')
        ax1.plot(time, omega, 'r-', linewidth=2, label='ω (rad/s)')
        ax1.set_xlabel('Tempo (s)')
        ax1.set_ylabel('Velocidade Linear e Angular')
        ax1.set_title('Sinal u(t)')
        ax1.grid(True, alpha=0.3)
        ax1.legend()
        
        ax2.plot(time, xf, 'g--', linewidth=2, label='xf (front)')
        ax2.plot(time, yf, 'b--', linewidth=2, label='yf (front)')
        ax2.set_xlabel('Tempo (s)')
        ax2.set_ylabel('Pos (m)')
        ax2.set_title('Posicao do Robo')
        ax2.grid(True, alpha=0.3)
        ax2.legend()
        
        ax3.plot(xf, yf, 'r-', linewidth=2, label='Trajeto do robo')
        ax3.plot(xf[0], yf[0], 'ro', markersize=6, label='Começo')
        ax3.plot(xf[-1], yf[-1], 'rs', markersize=6, label='Fim')
        ax3.set_xlabel('x (m)')
        ax3.set_ylabel('y (m)')
        ax3.set_title('Trajeto do robo')
        ax3.grid(True, alpha=0.3)
        ax3.legend()
        ax3.axis('equal')
        
        ax4.plot(time, theta_f * 180/np.pi, 'r-', linewidth=2, label='θ (degrees)')
        ax4.set_xlabel('Tempo (s)')
        ax4.set_ylabel('Theta em Graus')
        ax4.set_title('Posição angular do robo')
        ax4.grid(True, alpha=0.3)
        ax4.legend()
        
        plt.tight_layout()
        plt.savefig('robot_simn_plot.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("Figura salva em 'robot_sim_plot.png'")
        return True
        

def main():
    if len(sys.argv) != 2:
        print("Comando correto: python3 robust_plot_results.py robot_simulation.txt ou $(nome_do_arquivo.txt)")
        sys.exit(1)
    filename = sys.argv[1]
    
    try:
        data = read_simulation_data(filename)
        plot(data)
    except Exception as e:
        print(f"erro no plot: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()