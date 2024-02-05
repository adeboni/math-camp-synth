import numpy as np
from pyquaternion import Quaternion
from matplotlib import pyplot as plt
from matplotlib import animation

HUMAN_HEIGHT = 5
SIDE_LENGTH = 39
LASER_PROJECTION_ANGLE = 45 * np.pi / 180

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.set_xlim((-25, 25))
ax.set_ylim((-25, 25))
ax.set_zlim((0, 40))
ax.view_init(20, 50)

triangle_height = np.sqrt(SIDE_LENGTH**2 - (SIDE_LENGTH/2)**2)
tetra_height = SIDE_LENGTH * np.sqrt(2/3)
projection_bottom = tetra_height / 4
projection_top = tetra_height / 2

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
    np.array([find_edge_pos(edges[3], projection_bottom), find_edge_pos(edges[3], projection_top), 
              find_edge_pos(edges[4], projection_top), find_edge_pos(edges[4], projection_bottom)]),
    np.array([find_edge_pos(edges[3], projection_bottom), find_edge_pos(edges[3], projection_top), 
              find_edge_pos(edges[5], projection_top), find_edge_pos(edges[5], projection_bottom)]),
    np.array([find_edge_pos(edges[4], projection_bottom), find_edge_pos(edges[4], projection_top), 
              find_edge_pos(edges[5], projection_top), find_edge_pos(edges[5], projection_bottom)]),
]

for surface in surfaces:
    x = [s[0] for s in surface]
    y = [s[1] for s in surface]
    z = [s[2] for s in surface]
    ax.plot_trisurf(x, y, z, color='y', alpha=0.2)

def find_surface_normal(surface):
    pn = np.cross(surface[1] - surface[0], surface[2] - surface[0])
    return pn / np.linalg.norm(pn)

lines = sum([ax.plot([], [], [], c=c) for c in ['r', 'g', 'b']], [])
startpoints = np.array([[-2, 0, 0], [0, -2, 0], [0, 0, -2]])
endpoints =  np.array([[2, 0, 0], [0, 2, 0], [0, 0, 2]])

projection = ax.plot([], [], [], c='r', linestyle='', marker='o')
plane_normals = np.array([find_surface_normal(surface) for surface in surfaces])
plane_points = np.array([surface[0] for surface in surfaces])

lasers = [
    np.array(find_edge_pos(edges[5], projection_bottom)),
    np.array(find_edge_pos(edges[4], projection_bottom)),
    np.array(find_edge_pos(edges[3], projection_bottom))
]

laser_centers = []
for laser, pn, pp in zip(lasers, plane_normals, plane_points):
    ax.plot([laser[0]], [laser[1]], [laser[2]], c='k', linestyle='', marker='o', alpha=0.2)
    laser_center = laser - np.dot(laser - pp, pn) * pn
    laser_centers.append(laser_center)
    ax.plot([laser[0], laser_center[0]], [laser[1], laser_center[1]], [laser[2], laser_center[2]], color='k', alpha=0.2)
laser_distance = np.linalg.norm(laser_center - laser)
half_width = laser_distance * np.tan(LASER_PROJECTION_ANGLE)

def point_in_triangle(a, b, c, p):
    same_side = lambda p1, p2, a, b: np.dot(np.cross(b - a, p1 - a), np.cross(b - a, p2 - a)) >= 0
    return same_side(p, a, b, c) and same_side(p, b, a, c) and same_side(p, c, a, b)

def point_in_surface(s, p):
    return point_in_triangle(s[0], s[1], s[2], p) or point_in_triangle(s[2], s[3], s[0], p)

def find_quat(start, end):
    cross = np.cross(start, end)
    axis = cross / np.linalg.norm(cross)
    theta = np.arctan(np.linalg.norm(cross) / np.dot(start, end))
    return Quaternion(axis=axis, angle=theta)

center_line = [(vertices[0] + vertices[2]) / 2, (0, 0, tetra_height)]
center_point = find_edge_pos(center_line, projection_bottom + (projection_top - projection_bottom) / 2)
target_vector = np.array([center_point[0], center_point[1], center_point[2] - HUMAN_HEIGHT])
ax.plot([center_point[0]], [center_point[1]], [center_point[2]], c='b', linestyle='', marker='o')

tp_orig = np.array([
    [0, 2048, 0],
    [2048, 4095, 0],
    [4095, 2048, 0],
    [2048, 0, 0],
])
transforms = []
for idx, laser_center, pn in zip(range(3), laser_centers, plane_normals):
    v1 = np.array([-pn[1], pn[0], 0])
    v1 = v1 / np.linalg.norm(v1)
    v2 = np.cross(pn, v1) 
    # fix rotation on last one
    tp = np.array([laser_center + half_width * (np.cos(i * np.pi / 2) * v1 + np.sin(i * np.pi / 2) * v2) for i in range(4)])
    # for i, c in enumerate(['r', 'g', 'b', 'y']):
    #     ax.plot([tp[i][0]], [tp[i][1]], [tp[i][2]], c=c, linestyle='', marker='o')
    a = np.concatenate((tp_orig, np.ones((tp_orig.shape[0], 1))), axis=1)
    b = np.concatenate((tp, np.ones((tp.shape[0], 1))), axis=1)
    t, _, _, _ = np.linalg.lstsq(a, b, rcond=None)
    transforms.append(t.T)

laser_draw_width = 400
laser_lines = np.array([
    [[2048 - laser_draw_width, 2048 - laser_draw_width, 0, 1], [2048 - laser_draw_width, 2048 + laser_draw_width, 0, 1]],
    [[2048 - laser_draw_width, 2048 + laser_draw_width, 0, 1], [2048 + laser_draw_width, 2048 + laser_draw_width, 0, 1]],
    [[2048 + laser_draw_width, 2048 + laser_draw_width, 0, 1], [2048 + laser_draw_width, 2048 - laser_draw_width, 0, 1]],
    [[2048 + laser_draw_width, 2048 - laser_draw_width, 0, 1], [2048 - laser_draw_width, 2048 - laser_draw_width, 0, 1]]
])

for laser_line in laser_lines:
    for i in range(3):
        p1 = np.dot(transforms[i], laser_line[0])
        p2 = np.dot(transforms[i], laser_line[1])
        ax.plot([p1[0], p2[0]], [p1[1], p2[1]], [p1[2], p2[2]], c='b', alpha=0.5)


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
                point = end + (np.dot(pp - end, pn / np.dot(v, pn)) * v)
                if v[2] > 0 and point_in_surface(s, point):
                    projection[0].set_data([point[0]], [point[1]])
                    projection[0].set_3d_properties([point[2]])
    return lines

ani = animation.FuncAnimation(fig, animate, interval=25, cache_frame_data=False)
plt.show()
