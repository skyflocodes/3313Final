import pygame
import sys
from socket import socket, AF_INET, SOCK_STREAM
import random

# Initialize Pygame and the clock
pygame.init()
clock = pygame.time.Clock()

# Screen setup
size = (800, 600)
screen = pygame.display.set_mode(size)
pygame.display.set_caption("Client View")

# Asteroid setup
asteroid = pygame.Rect(random.randint(0, 750), 0, 50, 50)
asteroid_speed = 2

# Character setup
character = pygame.Rect(400, 300, 50, 50)
character_speed = 5

# Player alive flag
isAlive = True

# Function to reset the game
def reset_game():
    global character, asteroid, isAlive
    character.x = 400
    asteroid.y = 0
    asteroid.x = random.randint(0, 750)
    isAlive = True  # Confirm isAlive as global and reset it

def main():
    global isAlive  # Ensure isAlive is accessible globally
    server_address = ('localhost', 3007)
    with socket(AF_INET, SOCK_STREAM) as sock:
        try:
            sock.connect(server_address)
            done = False
            while not done:
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        done = True

                keys = pygame.key.get_pressed()
                if isAlive:
                    if keys[pygame.K_LEFT]:
                        character.x -= character_speed
                    if keys[pygame.K_RIGHT]:
                        character.x += character_speed

                # Determine what to send based on isAlive status
                message = str(character.x) if isAlive else "-999"
                sock.sendall(message.encode('utf-8'))

                character.left = max(character.left, 0)
                character.right = min(character.right, size[0])

                asteroid.y += asteroid_speed
                if asteroid.y > 600:
                    asteroid.y = 0
                    asteroid.x = random.randint(0, 750)

                if isAlive and character.colliderect(asteroid):
                    isAlive = False  # No longer alive, next send will be -999

                screen.fill((0, 0, 128))  # Always update the game scene
                pygame.draw.rect(screen, (255, 0, 0), asteroid)  # Draw asteroids

                if isAlive:  # Draw the character only if alive
                    pygame.draw.rect(screen, (255, 255, 255), character)

                pygame.display.flip()
                clock.tick(30)
        except ConnectionResetError:
            print("Connection was reset by the server.")
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
    pygame.quit()
