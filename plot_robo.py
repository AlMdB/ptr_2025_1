import matplotlib.pyplot as plt
import numpy as np

data = np.loadtxt('robot_simulation.txt', skiprows=1) 
t = data[:, 0]
x_ref = data[:, 1]
y_ref = data[:, 2]
y_bot_x = data[:, 3]
y_bot_y = data[:, 4]
xf = data[:, 5]
yf = data[:, 6]
theta = data[:, 7]

plt.figure(figsize=(10, 8))
plt.plot(x_ref, y_ref, 'b--', label="Trajetoria Referencia" (x_ref, y_ref))
plt.plot(xf, yf, 'r-', label='Pos Robo (xf, yf)')
plt.xlabel('Posicao X(m)')
plt.ylabel('Posicao Y (m)')
plt.title('Trajetoria Feita vs Referencia')
plt.legend()
plt.grid(True)
plt.axis('equal')
plt.show()


plt.figure(figsize=(10, 8))
plt.subplot(2, 1, 1)
plt.plot(t, x_ref, 'b--', label='x_ref')
plt.plot(t, xf, 'r-', label='xf')
plt.xlabel('Tempo(s)')
plt.ylabel('Posicao X(m)')
plt.title('Posicao X vs Tempo')
plt.legend()
plt.grid(True)

plt.subplot(2, 1, 2)
plt.plot(t, y_ref, 'b--', label='y_ref')
plt.plot(t, yf, 'r-', label='yf')
plt.xlabel('Tempo (s)')
plt.ylabel('Posicao Y (m)')
plt.title('Posicao Y vs Tempo')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()