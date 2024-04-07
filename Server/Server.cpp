#include <iostream>
#include <vector>
#include <mutex>
#include "thread.h" // Assuming Thread class is defined here
#include "socketserver.h" // Assuming SocketServer class is defined here
#include "SharedObject.h"
#include "Semaphore.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <random>

using namespace Sync;

class Asteroid {
public:
    int positionX;
    int speed;

    Asteroid(int posX, int spd) : positionX(posX), speed(spd) {}

    void Move() { positionX += speed; }
};

std::vector<Asteroid> asteroids;
std::mutex asteroidMutex;

class Player {
public:
    int id;
    int positionX = 0;
    Socket& connection;
    Player(Socket& conn) : connection(conn) {}
    int GetPositionX() const { return positionX; }
};

std::vector<Player*> players;
std::mutex playersMutex;

void BroadcastAsteroidPositions() {
    std::lock_guard<std::mutex> lock(asteroidMutex);
    for (const auto& asteroid : asteroids) {
        std::string asteroidMessage = "Asteroid position: " + std::to_string(asteroid.positionX) + ", Speed: " + std::to_string(asteroid.speed);
        for (const auto& player : players) {
            player->connection.Write(ByteArray(asteroidMessage)); // Assuming ByteArray can be constructed with std::string
        }
    }
}

void UpdateAsteroids() {
    std::lock_guard<std::mutex> lock(asteroidMutex);
    for (auto& asteroid : asteroids) {
        asteroid.Move();
    }
}

void GenerateAsteroid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100); // Random position
    std::uniform_int_distribution<> speedDis(1, 5); // Random speed

    asteroids.push_back(Asteroid(dis(gen), speedDis(gen)));
}

class ClientHandler : public Thread {
private:
    bool& shouldTerminate;
    Socket& connectionSocket;
    std::thread handlerThread;

public:
    ClientHandler(Socket& clientSocket, bool& terminateSignal)
        : connectionSocket(clientSocket), shouldTerminate(terminateSignal) {}

    void Start() {
        handlerThread = std::thread([this] { this->ThreadMain(); });
    }

    void SendPlayerPositions() {
        std::lock_guard<std::mutex> lock(playersMutex);
        for (const auto& player : players) {
            std::string positionMessage = "Player " + std::to_string(player->id) + " position: " + std::to_string(player->GetPositionX());
            connectionSocket.Write(ByteArray(positionMessage)); // Assuming ByteArray can be constructed with std::string
        }
    }

    virtual long ThreadMain() {
        while (!shouldTerminate) {
            try {
                SendPlayerPositions();

                ByteArray receivedData;
                connectionSocket.Read(receivedData);
                std::string move = receivedData.ToString(); // Assuming ToString method exists to convert ByteArray to std::string

                // Log received data to the console
                std::cout << "Received data: " << move << std::endl;

                std::lock_guard<std::mutex> lock(playersMutex);
            }
            catch (...) {
                shouldTerminate = true;
            }
        }
        return 0;
    }
};

class ConnectionManager : public Thread {
private:
    bool terminateFlag = false;
    SocketServer& serverInstance;

public:
    ConnectionManager(SocketServer& server) : serverInstance(server) {}

    virtual long ThreadMain() {
        while (!terminateFlag) {
            try {
                Socket* clientSocket = new Socket(serverInstance.Accept());

                std::lock_guard<std::mutex> lock(playersMutex);
                Player* newPlayer = new Player();
                newPlayer->id = players.size() + 1;
                players.push_back(newPlayer);

                ClientHandler* newHandler = new ClientHandler(*clientSocket, terminateFlag);
                newHandler->Start();
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

    // Background thread for generating asteroids and broadcasting their positions
    std::thread asteroidThread([] {
        while (true) {
            GenerateAsteroid();
            UpdateAsteroids();
            BroadcastAsteroidPositions();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    std::cin.get();

    std::cout << "Shutting down the server." << std::endl;
    server.Shutdown();

    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto* player : players) {
        delete player;
    }
    players.clear();

    asteroidThread.join();

    return 0;
}