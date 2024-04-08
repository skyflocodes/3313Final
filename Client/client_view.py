import pygame
import sys
from socket import socket, AF_INET, SOCK_STREAM
import random
import json

# Initialize Pygame and the clock
pygame.init()
clock = pygame.time.Clock()

# Screen setup
size = (800, 600)
screen = pygame.display.set_mode(size)
pygame.display.set_caption("Client View")

### Asteroid setup
##asteroid = pygame.Rect(random.randint(0, 750), 0, 50, 50)
##asteroid_speed = 100

# Character setup
character = pygame.Rect(400, 300, 50, 50)
character_speed = 100

# Player alive flag
isAlive = True

#Connection secret
secret = "monkey_eating_lettuce"

# Before the game loop starts
last_frame_time = pygame.time.get_ticks()


def main():
    global isAlive, asteroid_speed,last_frame_time # Ensure isAlive is accessible globally
    server_address = ('localhost', 2005)

    # Initialize and connect the socket
    sock = socket(AF_INET, SOCK_STREAM)
    try:
        sock.connect(server_address)
        sock.setblocking(0)  # Set the socket to non-blocking mode

        #send secret to server to have it recognize client as legitimate
        sock.sendall(secret.encode('utf-8'))
        
        # Initially placing the asteroid off-screen or at a default position
        asteroid = pygame.Rect(-100, -100, 50, 50)  # Starting off-screen
        
        done = False
        while not done:
            
            current_frame_time = pygame.time.get_ticks()
            delta_time = (current_frame_time - last_frame_time) / 1000.0
            last_frame_time = current_frame_time
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    done = True

            keys = pygame.key.get_pressed()
            if isAlive:
                if keys[pygame.K_LEFT] and character.x>=0:
                    character.x -= character_speed * delta_time
                if keys[pygame.K_RIGHT] and character.x<=(size[0]-50):
                    character.x += character_speed * delta_time



            # Send character's position to the server
            # Only send character's position to the server if isAlive is True
            if isAlive:
                message = str(character.x)
                print(message)
                sock.sendall(message.encode('utf-8'))


            # Attempt to receive and parse messages from the server
        try:
            message = sock.recv(1024).decode('utf-8')
            data = json.loads(message)
            if "positionX" in data and "positionY" in data:
                asteroid.x = data["positionX"]
                asteroid.y = data["positionY"]
                # Print the received asteroid position data
                print("Received asteroid position: X = ", asteroid.x, ", Y = ", asteroid.y)
            # Handle other data types as needed
        except BlockingIOError:
            pass  # No data received
        except Exception as e:
            print(f"An error occurred while receiving data: {e}")

            # # Update asteroid position based on its speed
            # asteroid.y += asteroid_speed * delta_time  # Move asteroid based on speed and delta time
            # if asteroid.y > 600:
            #     # Optionally reset asteroid's position to top if needed
            #     asteroid.y = 0

            # Check for collision between character and asteroid
            if isAlive and character.colliderect(asteroid):
                isAlive = False  # Character is no longer alive

            # Drawing and refreshing the screen
            screen.fill((0, 0, 128))
            pygame.draw.rect(screen, (255, 0, 0), asteroid)
            if isAlive:
                pygame.draw.rect(screen, (255, 255, 255), character)
            pygame.display.flip()
            clock.tick(30)
    except ConnectionResetError:
        print("Connection was reset by the server.")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
    pygame.quit()


class Asteroid(pygame.sprite.Sprite):

    def __init__(self):
        pygame.sprite.Sprite.__init__(self)
        self.rect = pygame.Rect(0, 600, 50, 50)
        
    #update function for grades; moves grades and kills grades if they reach bottom of screen
    def update(self,x,y):
        self.rect.x=x
        self.rect.y=y

class Player(pygame.sprite.Sprite):
    
    def __init__(self):
        pygame.sprite.Sprite.__init__(self)
        self.rect = pygame.Rect(400, 300, 50, 50)
        self.character_speed = 100
        self.isAlive = True

class Game():

    def __init__(self,screen):
        self.player = Player()
        self.asteroid = Asteroid()
        self.screen = screen
        self.clock = pygame.time.Clock()

        secret = "monkey_eating_lettuce"

        server_address = ('localhost', 2005)
        self.sock = socket(AF_INET, SOCK_STREAM)
        try:
            self.sock.connect(server_address)
            self.sock.setblocking(0)  # Set the socket to non-blocking mode

            #send secret to server to have it recognize client as legitimate
            self.sock.sendall(secret.encode('utf-8'))
        except BlockingIOError:
            pass  # No data received
        except Exception as e:
            print(f"An error occurred while receiving data: {e}")



    def render(self):
        if self.player.isAlive:
            self.screen.blit(pygame.Surface((50,50)),self.player.rect)
        self.screen.blit(pygame.Surface((50,50)),self.asteroid.rect)

        pygame.display.flip()
    
    def start(self):
        running = True

        self.render()

        
        last_frame_time = 0

        while running:
            
            #Player movement
            current_frame_time = pygame.time.get_ticks()
            delta_time = (current_frame_time - last_frame_time) / 1000.0
            last_frame_time = current_frame_time

            keys = pygame.key.get_pressed()
            if self.player.isAlive:
                if keys[pygame.K_LEFT] and self.player.rect.x>=0:
                    self.player.rect.x -= self.player.character_speed * delta_time
                elif keys[pygame.K_RIGHT] and self.player.rect.x<=(size[0]-50):
                    self.player.rect.x += self.player.character_speed * delta_time

            #Send player position to server
            if self.player.isAlive:
                message = str(self.player.rect.x)
                try:
                    self.sock.sendall(message.encode('utf-8'))
                except BlockingIOError:
                    pass  # No data received
                except Exception as e:
                    print(f"An error occurred while receiving data: {e}")

            #Recieve asteroid position from server
            try:
                message = self.sock.recv(1024).decode('utf-8')
                data = json.loads(message)
                if "positionX" in data and "positionY" in data:
                    self.asteroid.rect.x = data["positionX"]
                    self.asteroid.rect.y = data["positionY"]
                    # Print the received asteroid position data
                    print("Received asteroid position: X = ", data["positionX"], ", Y = ", data["positionY"])
                # Handle other data types as needed
            except BlockingIOError:
                pass  # No data received
            except Exception as e:
                print(f"An error occurred while receiving data: {e}")

            #Collision detection
            if self.asteroid.rect.colliderect(self.player.rect):
                self.player.isAlive=False
            
            self.render()
            clock.tick(30)
        
        self.sock.close()
        pygame.quit()
