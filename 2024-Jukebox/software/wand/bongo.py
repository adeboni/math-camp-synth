import os
import pygame
from pyquaternion import Quaternion
import time
import random

pygame.init()

SPEED_THRESHOLD = 0.4
NUM_WANDS = 2

files = []
for file in os.listdir("."):
    if file.endswith(".wav"):
        files.append(pygame.mixer.Sound(file))

wand_offset = Quaternion(1, 0, 0, -1)
vector = [1, 0, 0]
wands = [pygame.joystick.Joystick(i) for i in range(NUM_WANDS)]
queues = [[] for i in range(NUM_WANDS)]
prev_speeds = [0 for i in range(NUM_WANDS)]

def check_for_hit(wand_num):
    q = Quaternion(w=wands[wand_num].get_axis(5), x=wands[wand_num].get_axis(0), y=wands[wand_num].get_axis(1), z=wands[wand_num].get_axis(2))
    q = wand_offset.rotate(q)
    pos = q.rotate(vector)[2]
    if len(queues[wand_num]) > 10:
        queues[wand_num].pop(0)
    queues[wand_num].append(pos)
    new_speed = queues[wand_num][-1] - queues[wand_num][0]
    if prev_speeds[wand_num] > SPEED_THRESHOLD and new_speed < SPEED_THRESHOLD:
        files[random.randrange(len(files))].play()
    prev_speeds[wand_num] = new_speed

while True:
    pygame.event.pump()
    for i in range(NUM_WANDS):
        check_for_hit(i)
    time.sleep(0.02)
