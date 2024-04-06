import pygame
import sys
from socket import socket, AF_INET, SOCK_STREAM
import random  # Moved import to the top for better style

# Initialize Pygame and the clock
pygame.init()
clock = pygame.time.Clock()

# Screen setup
size = (800, 600)
screen = pygame.display.set_mode(size)
pygame.display.set_caption("Client View")

# Asteroid setup
asteroid = pygame.Rect(random.randint(0, 750), 0, 50, 50)  # Random initial position at the top
asteroid_speed = 2  # Pixels the asteroid moves per frame

# Function to reset the game
def reset_game():
    global character, asteroid
    character.x = 400  # Reset player to the middle
    asteroid.y = 0  # Reset asteroid to the top
    asteroid.x = random.randint(0, 750)  # New random x position for the asteroid

# Character setup
character = pygame.Rect(400, 300, 50, 50)  # Initial position of the character
character_speed = 5  # Pixels the character moves per frame

def main():
    server_address = ('localhost', 3007)
    with socket(AF_INET, SOCK_STREAM) as sock:
        try:
            sock.connect(server_address)
            done = False
            while not done:
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        done = True

                # Movement and message sending
                keys = pygame.key.get_pressed()
                if keys[pygame.K_LEFT]:
                    character.x -= character_speed
                    sock.sendall("move left".encode('utf-8'))
                if keys[pygame.K_RIGHT]:
                    character.x += character_speed
                    sock.sendall("move right".encode('utf-8'))

                # Keep the character on screen
                character.left = max(character.left, 0)
                character.right = min(character.right, size[0])

                # Move asteroid and check for collision within the loop
                asteroid.y += asteroid_speed
                if asteroid.y > 600:  # If asteroid moves off screen, reset its position
                    asteroid.y = 0
                    asteroid.x = random.randint(0, 750)

                # Check for collision
                if character.colliderect(asteroid):
                    reset_game()  # Reset the game on collision

                # Drawing
                screen.fill((0, 0, 128))  # Blue background
                pygame.draw.rect(screen, (255, 0, 0), asteroid)  # Draw the asteroid
                pygame.draw.rect(screen, (255, 255, 255), character)  # Draw the character
                pygame.display.flip()
                clock.tick(30)  # FPS limit
        except ConnectionResetError:
            print("Connection was reset by the server.")
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
    pygame.quit()
