import math
import numpy as np
from pyquaternion import Quaternion
from matplotlib import pyplot as plt
from matplotlib import animation

HUMAN_HEIGHT = 5
SIDE_LENGTH = 40
PROJECTION_BOTTOM = 5
PROJECTION_TOP = 25

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.set_xlim((-25, 25))
ax.set_ylim((-25, 25))
ax.set_zlim((0, 40))
ax.view_init(20, 50)

triangle_height = math.sqrt(SIDE_LENGTH**2 - (SIDE_LENGTH/2)**2)
tetra_height = SIDE_LENGTH * math.sqrt(2/3)

vertices = [
    np.array([-SIDE_LENGTH/2, -triangle_height/3, 0]),
    np.array([SIDE_LENGTH/2, -triangle_height/3, 0]),
    np.array([0, triangle_height*2/3, 0]),
    np.array([0, 0, tetra_height]),
]

edges = [
    (vertices[0], vertices[1]),
    (vertices[0], vertices[2]),
    (vertices[1], vertices[2]),
    (vertices[1], vertices[3]),
    (vertices[0], vertices[3]),
    (vertices[2], vertices[3])
]

for e1, e2 in edges:
    ax.plot([e1[0], e2[0]], [e1[1], e2[1]], [e1[2], e2[2]], color='k')

def find_edge_pos(edge, z):
    x = edge[0][0] + (z - edge[0][2]) * (edge[1][0] - edge[0][0]) / (edge[1][2] - edge[0][2])
    y = edge[0][1] + (z - edge[0][2]) * (edge[1][1] - edge[0][1]) / (edge[1][2] - edge[0][2])
    return (x, y, z)

surfaces = [
    np.array([find_edge_pos(edges[3], PROJECTION_BOTTOM), find_edge_pos(edges[3], PROJECTION_TOP), 
              find_edge_pos(edges[4], PROJECTION_TOP), find_edge_pos(edges[4], PROJECTION_BOTTOM)]),
    np.array([find_edge_pos(edges[3], PROJECTION_BOTTOM), find_edge_pos(edges[3], PROJECTION_TOP), 
              find_edge_pos(edges[5], PROJECTION_TOP), find_edge_pos(edges[5], PROJECTION_BOTTOM)]),
    np.array([find_edge_pos(edges[4], PROJECTION_BOTTOM), find_edge_pos(edges[4], PROJECTION_TOP), 
              find_edge_pos(edges[5], PROJECTION_TOP), find_edge_pos(edges[5], PROJECTION_BOTTOM)]),
]

for surface in surfaces:
    x = [s[0] for s in surface]
    y = [s[1] for s in surface]
    z = [s[2] for s in surface]
    ax.plot_trisurf(x, y, z, color='y', alpha=0.2)

def find_surface_normal(surface):
    return np.cross(surface[1] - surface[0], surface[2] - surface[0])

lines = sum([ax.plot([], [], [], c=c) for c in ['r', 'g', 'b']], [])
startpoints = np.array([[-2, 0, 0], [0, -2, 0], [0, 0, -2]])
endpoints =  np.array([[2, 0, 0], [0, 2, 0], [0, 0, 2]])

projection = ax.plot([], [], [], c='r', linestyle='', marker='o')
plane_normals = np.array([find_surface_normal(surface) for surface in surfaces])
plane_points = np.array([surface[0] for surface in surfaces])

def point_in_triangle(a, b, c, p):
    same_side = lambda p1, p2, a, b: np.cross(b - a, p1 - a).dot(np.cross(b - a, p2 - a)) >= 0
    return same_side(p, a, b, c) and same_side(p, b, a, c) and same_side(p, c, a, b)

def point_in_surface(s, p):
    return point_in_triangle(s[0], s[1], s[2], p) or point_in_triangle(s[2], s[3], s[0], p)

def find_quat(start, end):
    cross = np.cross(start, end)
    axis = cross / np.linalg.norm(cross)
    theta = np.arctan(np.linalg.norm(cross) / np.dot(start, end))
    return Quaternion(axis=axis, angle=theta)

center_line = [(vertices[0] + vertices[2]) / 2, (0, 0, tetra_height)]
center_point = find_edge_pos(center_line, PROJECTION_BOTTOM + (PROJECTION_TOP - PROJECTION_BOTTOM) / 2)
target_vector = np.array([center_point[0], center_point[1], center_point[2] - HUMAN_HEIGHT])
ax.plot([center_point[0]], [center_point[1]], [center_point[2]], c='b', linestyle='', marker='o')

def joystick_quaternion():
    import pygame
    pygame.init()
    controller = pygame.joystick.Joystick(0)
    wand_offset = Quaternion(1, 0, 0, -1)
    while True:
        pygame.event.pump()
        q = Quaternion(w=controller.get_axis(5), x=controller.get_axis(0), y=controller.get_axis(1), z=controller.get_axis(2))
        yield wand_offset.rotate(q)

def mouse_quaternion():
    import pyautogui
    start = np.array([1, 0, 0])
    mouse_offset = find_quat(start, target_vector)
    while True:
        mouse = pyautogui.position()
        end = np.array([1, np.interp(mouse.x, [0, 1920], [1, -1]), np.interp(mouse.y, [0, 1080], [-1, 1])])
        yield mouse_offset * find_quat(start, end)

quaternion_generator = mouse_quaternion()

def animate(_):
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
