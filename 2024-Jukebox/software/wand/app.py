import pygame

pygame.init()
pygame.joystick.init()
controller = pygame.joystick.Joystick(0)
controller.init()

print(f"Name of the joystick: {controller.get_name()}")
print(f"Number of axis: {controller.get_numaxes()}")

while True:
	pygame.event.pump()
	print([f"{controller.get_axis(i):.2f}" for i in range(8)])