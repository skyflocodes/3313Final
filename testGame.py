import pygame
import sys
from socket import socket, AF_INET, SOCK_STREAM
import random
import json

pygame.init()
#top left of the screen is at coordinate (0,0)
size = (800, 600)
clock = pygame.time.Clock()

class Asteroid(pygame.sprite.Sprite):

    #given random x value, spawn asteroid at random x and top of screen
    def __init__(self,x):
        pygame.sprite.Sprite.__init__(self)
        self.rect = pygame.Rect(x,0, 50, 50)
        self.image = pygame.Surface((50,50))
        self.image.fill((255, 255, 0))
        self.speed = 5
        
    #update function for grades; moves grades and kills grades if they reach bottom of screen
    def fall(self):
        self.rect.y+=self.speed
        #if asteroid reaches ground, kill
        if self.rect.y+self.rect.height>=650:
            print("Asteroid has made contact...")
            self.kill()

class Player(pygame.sprite.Sprite):
    
    def __init__(self):
        pygame.sprite.Sprite.__init__(self)
        #start at (400,300)
        self.rect = pygame.Rect(375,size[1]-50, 50, 50)
        self.image = pygame.Surface((50,50))
        self.image.fill((255, 0, 0))  # Fill the surface with red color
        self.character_speed = 100
        self.isAlive = True

class OtherPlayer(pygame.sprite.Sprite):
    
    def __init__(self,x):
        pygame.sprite.Sprite.__init__(self)
        #start at (x,0)
        self.rect = pygame.Rect(x,size[1]-50,50,50)
        self.image = pygame.Surface((50,50))
        self.image.fill((0, 255, 0))  # Fill the surface with green color, semitransparent
        self.image.set_alpha(128)


class Game():


    def __init__(self,screen):
        self.player = Player()
        self.asteroids = pygame.sprite.Group()
        self.other_players = pygame.sprite.Group()

        self.last_asteroid_spawn = pygame.time.get_ticks()
        self.screen = screen
        self.clock = pygame.time.Clock()


    def render(self):
        self.screen.fill((0, 0, 255))  # Fill the screen with black color

        self.other_players.draw(self.screen)  # Draw other players

        if self.player.isAlive:
            self.screen.blit(self.player.image, self.player.rect)

        for asteroid in self.asteroids:
            asteroid.fall()
        self.asteroids.draw(self.screen)  # Draw asteroids

        pygame.display.flip()

    def spawnAsteroid(self,position):
        asteroid = Asteroid(position)
        self.asteroids.add(asteroid)

    def spawnOtherPlayers(self, positions):
        if len(positions)==len(self.other_players):
            #iterate through positions, set new position(x) for each respective OtherPlayer
            for other_player, pos in zip(self.other_players, positions):
                other_player.rect.x = pos
        else:
            #clear OtherPlayers, iterate through positions, create new OtherPlayer, place them at new positions
            self.other_players.empty()
            for pos in positions:
                other_player = OtherPlayer(pos)
                self.other_players.add(other_player)
            
        
    
    def start(self):
        running = True

        self.render()

        
        last_frame_time = 0
        
        print("Game Starting:")
        while self.player.isAlive or len(self.other_players)!=0:
            #Player movement
            current_frame_time = pygame.time.get_ticks()
            delta_time = (current_frame_time - last_frame_time) / 1000.0
            last_frame_time = current_frame_time

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running=False
                #if a key pressed, move student left or right depending on input
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_LEFT and self.player.rect.x>=0:
                        print("Left")
                        self.player.rect.x -= self.player.character_speed * delta_time
                    elif event.key == pygame.K_RIGHT and self.player.rect.x<=(size[0]-50):
                        print("Right")
                        self.player.rect.x += self.player.character_speed * delta_time
            
            keys = pygame.key.get_pressed()
            if keys[pygame.K_a] and self.player.rect.x>=0:
                self.player.rect.x -= self.player.character_speed * delta_time
            elif keys[pygame.K_d] and self.player.rect.x<=(self.screen.get_rect().width-self.player.rect.width):
                self.player.rect.x += self.player.character_speed * delta_time

            #Collision detection
            if pygame.sprite.spritecollide(self.player,self.asteroids,dokill=True):
                self.player.isAlive=False
            
            self.render()
            clock.tick(30)
        
        pygame.quit()
        print("Graceful termination!")

# Main function
def main():
    # Screen setup
    screen = pygame.display.set_mode(size)
    pygame.display.set_caption("Client View")
    clock = pygame.time.Clock()
    game = Game(screen)

    # Example: positions of other players
    other_players_positions = []
    game.spawnOtherPlayers(other_players_positions)
    asteroid_positions = [100,150,250,371]
    for x in asteroid_positions:
        print("Spawning Asteroids...")
        game.spawnAsteroid(x)

    game.start()


if __name__ == "__main__":
    main()