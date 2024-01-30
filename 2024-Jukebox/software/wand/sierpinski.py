import math
import numpy as np
from pyquaternion import Quaternion
from matplotlib import pyplot as plt
from matplotlib import animation

HUMAN_HEIGHT = 5
SIDE_LENGTH = 40
PROJECTION_BOTTOM = 5
PROJECTION_TOP = 30
QUAT_OFFSET = Quaternion(1, 0, 0, -1)

def joystick_quaternion():
    import pygame
    pygame.init()
    controller = pygame.joystick.Joystick(0)
    while True:
        pygame.event.pump()
        q = Quaternion(w=controller.get_axis(5), x=controller.get_axis(0), y=controller.get_axis(1), z=controller.get_axis(2))
        yield QUAT_OFFSET.rotate(q)

def mouse_quaternion():
    import pyautogui
    while True:
        mouse = pyautogui.position()
        start = np.array([1, 0, 0])
        end = np.array([1, np.interp(mouse.x, [0, 1920], [1, -1]), np.interp(mouse.y, [0, 1080], [-1, 1])])
        axis = np.cross(start, end) / np.linalg.norm(np.cross(start, end))
        theta = np.arctan(np.linalg.norm(np.cross(start, end)) / np.dot(start, end))
        yield Quaternion(axis=axis, angle=theta)

quaternion_generator = mouse_quaternion()

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.set_xlim((-25, 25))
ax.set_ylim((-25, 25))
ax.set_zlim((0, 40))
ax.view_init(20, 50)

triangle_height = math.sqrt(SIDE_LENGTH**2 - (SIDE_LENGTH/2)**2)
sides = [
    [(-SIDE_LENGTH/2, -triangle_height/3, 0), (SIDE_LENGTH/2, -triangle_height/3, 0)],
    [(-SIDE_LENGTH/2, -triangle_height/3, 0), (0, triangle_height*2/3, 0)],
    [(SIDE_LENGTH/2, -triangle_height/3, 0), (0, triangle_height*2/3, 0)],
    [(SIDE_LENGTH/2, -triangle_height/3, 0), (0, 0, triangle_height)],
    [(-SIDE_LENGTH/2, -triangle_height/3, 0), (0, 0, triangle_height)],
    [(0, triangle_height*2/3, 0), (0, 0, triangle_height)],
]

for s1, s2 in sides:
    ax.plot([s1[0], s2[0]], [s1[1], s2[1]], [s1[2], s2[2]], color='k')

def find_edge_pos(edge, z):
    x = edge[0][0] + (z - edge[0][2]) * (edge[1][0] - edge[0][0]) / (edge[1][2] - edge[0][2])
    y = edge[0][1] + (z - edge[0][2]) * (edge[1][1] - edge[0][1]) / (edge[1][2] - edge[0][2])
    return (x, y, z)

def find_surface_normal(surface):
    ab = surface[1] - surface[0]
    ac = surface[2] - surface[0]
    return np.cross(ab, ac)

surfaces = [
    np.array([find_edge_pos(sides[3], PROJECTION_BOTTOM), find_edge_pos(sides[3], PROJECTION_TOP), 
              find_edge_pos(sides[4], PROJECTION_TOP), find_edge_pos(sides[4], PROJECTION_BOTTOM)]),
    np.array([find_edge_pos(sides[3], PROJECTION_BOTTOM), find_edge_pos(sides[3], PROJECTION_TOP), 
              find_edge_pos(sides[5], PROJECTION_TOP), find_edge_pos(sides[5], PROJECTION_BOTTOM)]),
    np.array([find_edge_pos(sides[4], PROJECTION_BOTTOM), find_edge_pos(sides[4], PROJECTION_TOP), 
              find_edge_pos(sides[5], PROJECTION_TOP), find_edge_pos(sides[5], PROJECTION_BOTTOM)]),
]

for surface in surfaces:
    x = [s[0] for s in surface]
    y = [s[1] for s in surface]
    z = [s[2] for s in surface]
    ax.plot_trisurf(x, y, z, color='y', alpha=0.2)

lines = sum([ax.plot([], [], [], c=c) for c in ['r', 'g', 'b']], [])
startpoints = np.array([[-2, 0, 0], [0, -2, 0], [0, 0, -2]])
endpoints =  np.array([[2, 0, 0], [0, 2, 0], [0, 0, 2]])

projection = ax.plot([], [], [], c='r', linestyle='', marker='o')
plane_normals = np.array([find_surface_normal(surfaces[0]), find_surface_normal(surfaces[1]), find_surface_normal(surfaces[2])])
plane_points = np.array([surfaces[0][0], surfaces[1][0], surfaces[2][0]])

def point_in_triangle(a, b, c, p):
    same_side = lambda p1, p2, a, b: np.cross(b - a, p1 - a).dot(np.cross(b - a, p2 - a)) >= 0
    return same_side(p, a, b, c) and same_side(p, b, a, c) and same_side(p, c, a, b)

def point_in_surface(s, p):
    return point_in_triangle(s[0], s[1], s[2], p) or point_in_triangle(s[2], s[3], s[0], p)

def animate(_):
    global last_press
    q = next(quaternion_generator)

    for i, (line, start, end) in enumerate(zip(lines, startpoints, endpoints)):
        start = q.rotate(start)
        end = q.rotate(end)
        start[2] += HUMAN_HEIGHT
        end[2] += HUMAN_HEIGHT
        line.set_data([start[0], end[0]], [start[1], end[1]])
        line.set_3d_properties([start[2], end[2]])
        if i == 0:
            projection[0].set_data([], [])
            projection[0].set_3d_properties([])
            for pn, pp, s in zip(plane_normals, plane_points, surfaces):
                v = start - end
                point = end + ((pp - end).dot(pn / v.dot(pn)) * v)
                if v[2] > 0 and point_in_surface(s, point):
                    projection[0].set_data([point[0]], [point[1]])
                    projection[0].set_3d_properties([point[2]])
    return lines

ani = animation.FuncAnimation(fig, animate, interval=25, cache_frame_data=False)
plt.show()
