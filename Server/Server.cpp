#include <iostream>
#include <vector>
#include <mutex>
#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <chrono>
#include <thread>
#include <random>
#include "nlohmann/json.hpp"
//you will need to install json

using json = nlohmann::json;
using namespace Sync; // Assuming Sync namespace is defined in one of the included headers

// Forward declaration of Socket to be used in Player and ClientHandler
class Socket;

class Asteroid {
public:
    int positionX;
    int positionY;
    int speed;

    Asteroid(int posX, int posY, int spd) : positionX(posX), positionY(posY), speed(spd) {}

    json toJson() const {
        json asteroidJson;
        asteroidJson["positionX"] = positionX;
        asteroidJson["positionY"] = positionY;
        return asteroidJson;
    }
};

std::mutex asteroidMutex;
Asteroid asteroid(0, 0, 5); // Initialize with some default values

void InitializeAsteroid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, 750); // Random position for X

    std::lock_guard<std::mutex> lock(asteroidMutex);
    asteroid.positionX = disX(gen);
    asteroid.positionY = 550; // Start from the top of the screen
}

class Player
{
public:
    int id;
    int positionX = 0;
    Socket &connection;
    Player(Socket &conn) : connection(conn) {}
    int GetPositionX() const { return positionX; }
};

std::vector<Player *> players;
std::mutex playersMutex;

void UpdateAsteroidPosition() {
    std::lock_guard<std::mutex> lock(asteroidMutex);
    asteroid.positionY -= asteroid.speed; // Move down by its speed

    if (asteroid.positionY < 0) { // If it reaches or passes the bottom
        InitializeAsteroid(); // Reset position and pick a new X
    }
}

void BroadcastAsteroidPositions() {
    UpdateAsteroidPosition(); // Update position before broadcasting

    json asteroidJson = asteroid.toJson();
    std::string asteroidMessage = asteroidJson.dump();
    std::cout << "Broadcasting " << asteroidMessage << std::endl;

    for (const auto& player : players) {
        player->connection.Write(ByteArray(asteroidMessage));
    }
}

class ClientHandler : public Thread
{
private:
    bool &shouldTerminate;
    Socket &connectionSocket;
    std::thread handlerThread;

public:
    ClientHandler(Socket &clientSocket, bool &terminateSignal)
        : connectionSocket(clientSocket), shouldTerminate(terminateSignal) {}

    void Start()
    {
        handlerThread = std::thread([this]
                                    { this->ThreadMain(); });
    }

    void SendPlayerPositions()
    {
        std::lock_guard<std::mutex> lock(playersMutex);

        json playerPositions;

        for (const auto &player : players)
        {
            playerPositions[std::to_string(player->id)]["positionX"] = player->positionX;

            // std::string positionMessage = "\nPlayer " + std::to_string(player->id) + " position: " + std::to_string(player->GetPositionX());
            // connectionSocket.Write(ByteArray(positionMessage)); // Assuming ByteArray can be constructed with std::string
        }

        std::string positionMessage = playerPositions.dump();
        connectionSocket.Write(ByteArray(positionMessage));
    }

    virtual long ThreadMain()
    {
        while (!shouldTerminate)
        {
            try
            {
                SendPlayerPositions();

                ByteArray receivedData;
                connectionSocket.Read(receivedData);
                std::string move = receivedData.ToString(); // Assuming ToString method exists to convert ByteArray to std::string

                // Log received data to the console
                std::cout << "Received data: " << move << std::endl;

                std::lock_guard<std::mutex> lock(playersMutex);
            }
            catch (...)
            {
                shouldTerminate = true;
            }
        }
        return 0;
    }
};

class ConnectionManager : public Thread
{
private:
    bool terminateFlag = false;
    SocketServer &serverInstance;

public:
    ConnectionManager(SocketServer &server) : serverInstance(server) {}

    virtual long ThreadMain()
    {
        ByteArray bytes;
        while (!terminateFlag)
        {
            try
            {
                Socket *clientSocket = new Socket(serverInstance.Accept());

                int r = clientSocket->Read(bytes);
                if (r == -1)
                {
                    std::cout << "Error in socket detected" << std::endl;
                    // no change
                }
                else if (r == 0)
                {
                    std::cout << "Socket closed at remote end" << std::endl;
                    break;
                }
                else
                {
                    std::string theString = bytes.ToString();

                    if (theString == "monkey_eating_lettuce")
                    {
                        std::lock_guard<std::mutex> lock(playersMutex);
                        Player *newPlayer = new Player(*clientSocket); // Corrected to pass the Socket reference

                        newPlayer->id = players.size() + 1;
                        players.push_back(newPlayer);

                        ClientHandler *newHandler = new ClientHandler(*clientSocket, terminateFlag);
                        newHandler->Start();
                    }
                }
            }
            catch (...)
            {
                terminateFlag = true;
            }
        }
        return 0;
    }
};

int main()
{
    std::cout << "Server is active. Listening on port pyt." << std::endl;
    std::cout << "Press Enter to shut down the server..." << std::endl;

    SocketServer server(2005);
    ConnectionManager serverManager(server);

    // Background thread for generating asteroids and broadcasting their positions
    std::thread asteroidThread([] {
        while (true) {
            BroadcastAsteroidPositions();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Control the update rate
        }
    });

    std::cin.get();

    std::cout << "Shutting down the server." << std::endl;
    server.Shutdown();

    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto *player : players)
    {
        delete player;
    }
    players.clear();

    asteroidThread.join();

    return 0;
}