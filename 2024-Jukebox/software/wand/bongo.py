import os
import pygame
from pyquaternion import Quaternion
import time
import random

SPEED_THRESHOLD = 0.4

files = []
for file in os.listdir("."):
    if file.endswith(".wav"):
        files.append(file)

vector = [1, 0, 0]
pygame.init()
wand = pygame.joystick.Joystick(0)
prev_press = 0
wand_offset = Quaternion(1, 0, 0, -1)
queue = []
prev_speed = 0

while True:
    pygame.event.pump()
    q = Quaternion(w=wand.get_axis(5), x=wand.get_axis(0), y=wand.get_axis(1), z=wand.get_axis(2))
    q = wand_offset.rotate(q)
    pos = q.rotate(vector)[2]
    if len(queue) > 10:
        queue.pop(0)
    queue.append(pos)
    new_speed = queue[-1] - queue[0]
    if prev_speed > SPEED_THRESHOLD and new_speed < SPEED_THRESHOLD:
        print("hit")
        pygame.mixer.music.load(files[random.randrange(len(files))])
        pygame.mixer.music.play()
    prev_speed = new_speed
    time.sleep(0.02)
