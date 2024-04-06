#include <vector>
#include <algorithm>
#include "thread.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <mutex>
#include "socketserver.h"

using namespace Sync;

// Forward declaration for Player class
class Player {
public:
    int positionX = 0; // Example player position property

    void MoveLeft() { positionX -= 1; } // Move the player left
    void MoveRight() { positionX += 1; } // Move the player right
    int GetPositionX() const { return positionX; } // Get player's X position
};



std::vector<Player*> players; // Global player list
std::mutex playersMutex; // Global mutex for synchronizing access to players
class Asteroid {
public:
    int positionX;
    int positionY;
    static const int speedY = 2; // Speed at which the asteroid moves down

    Asteroid(int posX, int posY) : positionX(posX), positionY(posY) {}

    void UpdatePosition() {
        positionY += speedY;
    }

};
// Assume we have a global list of asteroids
std::vector<Asteroid*> asteroids;
std::mutex asteroidsMutex;

// Handles communication with clients, processing their messages.
class ClientHandler : public Thread {

    
private:
    bool& shouldTerminate;
    Socket& connectionSocket;
    ByteArray receivedData;
    Player* player; // The Player object associated with this handler

public:
    ClientHandler(Socket& clientSocket, bool& terminateSignal, Player* playerObject)
        : connectionSocket(clientSocket), shouldTerminate(terminateSignal), player(playerObject) {}
    
    ~ClientHandler() {}




    Socket& GetConnectionSocket() { return connectionSocket; }

    virtual long ThreadMain() {
        while(!shouldTerminate) {
            try {
                connectionSocket.Read(receivedData);
                std::string originalMessage = receivedData.ToString();

                // Lock the mutex while updating the game state
                std::lock_guard<std::mutex> lock(playersMutex);

                if (originalMessage == "left") {
                    player->MoveLeft();
                }
                else if (originalMessage == "right") {
                    player->MoveRight();
                }

                std::string newPosition = "New Position: " + std::to_string(player->GetPositionX());
                receivedData = ByteArray(newPosition);
                connectionSocket.Write(receivedData);
                
            }
            catch (...) {
                shouldTerminate = true;
            }
        }
        return 0;
    }
};

// Manages server lifecycle and client connections.
class ConnectionManager : public Thread {
private:
    bool terminateFlag = false;
    SocketServer& serverInstance;
    std::vector<ClientHandler*> clientHandlers;

public:
    ConnectionManager(SocketServer& server) : serverInstance(server) {}

    ~ConnectionManager() {
        terminateFlag = true;
        for (auto& handler : clientHandlers) {
            if (handler) {
                handler->GetConnectionSocket().Close();
                delete handler;
                handler = nullptr;
            }
        }
        clientHandlers.clear();
    }

    virtual long ThreadMain() {
        while (!terminateFlag) {
            try {
                Socket* clientSocket = new Socket(serverInstance.Accept());

                // Create a new Player for each client
                std::lock_guard<std::mutex> lock(playersMutex);
                Player* newPlayer = new Player();
                players.push_back(newPlayer);

                ClientHandler* newHandler = new ClientHandler(*clientSocket, terminateFlag, newPlayer);
                clientHandlers.push_back(newHandler);
            }
            catch (...) {
                terminateFlag = true;
            }
        }
        return 0;
    }
};

int main() {
    std::cout << "Server is active. Listening on port 3007." << std::endl;
    std::cout << "Press Enter to shut down the server..." << std::endl;

    SocketServer server(3007);
    ConnectionManager serverManager(server);

    std::cin.get();

    std::cout << "Shutting down the server." << std::endl;
    server.Shutdown();

    // Cleanup global players list
    for (auto* player : players) {
        delete player;
    }
    players.clear();

    return 0;
}
