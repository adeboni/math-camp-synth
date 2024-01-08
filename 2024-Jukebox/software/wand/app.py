import pygame
import numpy as np
from pyquaternion import Quaternion
from matplotlib import pyplot as plt
from matplotlib import animation

fig = plt.figure()
ax = fig.add_axes([0, 0, 1, 1], projection='3d')
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')

lines = sum([ax.plot([], [], [], c=c) for c in ['r', 'g', 'b']], [])
startpoints = np.array([[0, 0, 0], [0, 0, 0], [0, 0, 0]])
endpoints = np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]])

ax.set_xlim((-2, 2))
ax.set_ylim((-2, 2))
ax.set_zlim((-2, 2))
ax.view_init(30, 30)

def animate(i):
    pygame.event.pump()
    q = Quaternion(w=controller.get_axis(5), x=controller.get_axis(0), y=controller.get_axis(1), z=controller.get_axis(2))
    for line, start, end in zip(lines, startpoints, endpoints):
        start = q.rotate(start)
        end = q.rotate(end)
        line.set_data([start[0], end[0]], [start[1], end[1]])
        line.set_3d_properties([start[2], end[2]])
    fig.canvas.draw()
    return lines

pygame.init()
pygame.joystick.init()
controller = pygame.joystick.Joystick(0)
controller.init()

anim = animation.FuncAnimation(fig, animate, interval=50, cache_frame_data=False, blit=True)
plt.show()
