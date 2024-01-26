import numpy as np
import pyautogui
import time
from pyquaternion import Quaternion
from matplotlib import pyplot as plt
from matplotlib import animation

BOUNDS = 2000
QUEUE_LIMIT = 10

def dist_3d(p1, p2):
    return sum(abs(p1[i] - p2[i]) for i in range(3))

def joystick_quaternion():
    import pygame
    pygame.init()
    pygame.joystick.init()
    controller = pygame.joystick.Joystick(0)
    controller.init()
    while True:
        pygame.event.pump()
        yield Quaternion(w=controller.get_axis(5), x=controller.get_axis(0), y=controller.get_axis(1), z=controller.get_axis(2))

def mouse_quaternion():
    while True:
        mouse = pyautogui.position()
        start = np.array([1, 0, 0])
        end = np.array([1, np.interp(mouse.x, [0, 1920], [1, -1]), np.interp(mouse.y, [0, 1080], [-1, 1])])
        axis = np.cross(start, end) / np.linalg.norm(np.cross(start, end))
        theta = np.arctan(np.linalg.norm(np.cross(start, end)) / np.dot(start, end))
        yield Quaternion(axis=axis, angle=theta)
    
quaternion_generator = joystick_quaternion()

fig = plt.figure()
ax = fig.add_axes([0, 0, 1, 1], projection='3d')
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_xlim((-BOUNDS, BOUNDS))
ax.set_ylim((-BOUNDS, BOUNDS))
ax.set_zlim((-BOUNDS, BOUNDS))
ax.view_init(30, 30)

target = ax.plot([], [], [], c='gray', alpha=0.5, linestyle='', marker='o', markersize=20)
trace = ax.plot([], [], [], c='gray')
trace_data = []
q_data = []
projections = sum([ax.plot([], [], [], c=c, linestyle='', marker='o') for c in ['r', 'g', 'b']], [])
lines = sum([ax.plot([], [], [], c=c) for c in ['r', 'g', 'b']], [])
startpoints = np.array([[-BOUNDS / 2, 0, 0], [0, -BOUNDS / 2, 0], [0, 0, -BOUNDS / 2]])
endpoints = np.array([[BOUNDS / 2, 0, 0], [0, BOUNDS / 2, 0], [0, 0, BOUNDS / 2]])
plane_normals = np.array([[-1, 0, 0], [0, -1, 0], [0, 0, -1]])
plane_points = np.array([[-BOUNDS, 0, 0], [0, -BOUNDS, 0], [0, 0, -BOUNDS]])
last_press = time.time()

def animate(_):
    global last_press
    q = next(quaternion_generator)
    if len(q_data) > QUEUE_LIMIT:
        q_data.pop(0)
    q_data.append([q.x, q.y, q.z])

    for i, (line, start, end, marker, pn, pp) in enumerate(zip(lines, startpoints, endpoints, projections, plane_normals, plane_points)):
        start = q.rotate(start)
        end = q.rotate(end)
        line.set_data([start[0], end[0]], [start[1], end[1]])
        line.set_3d_properties([start[2], end[2]])
        point = start - end * pn.dot(start - pp) / pn.dot(end)
        marker.set_data([point[0]], [point[1]])
        marker.set_3d_properties([point[2]])
        if i == 0:
            moved_enough = max(dist_3d(q_data[0], p) for p in q_data) > 0.5
            back_to_start = dist_3d(q_data[0], q_data[-1]) < 0.1

            if len(trace_data) > QUEUE_LIMIT:
                trace_data.pop(0)
            trace_data.append(start[:])
            trace[0].set_data([p[0] for p in trace_data], [p[1] for p in trace_data])
            trace[0].set_3d_properties([p[2] for p in trace_data])
            trace[0].set_color('black' if moved_enough else 'gray')
            target[0].set_data([trace_data[0][0]], [trace_data[0][1]])
            target[0].set_3d_properties([trace_data[0][2]])
            target[0].set_color('black' if back_to_start and moved_enough else 'gray')
            if back_to_start and moved_enough and time.time() - last_press > 1:
                last_press = time.time()
                print("Trigger")
    return lines

ani = animation.FuncAnimation(fig, animate, interval=25, cache_frame_data=False)
plt.show()
