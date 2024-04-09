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
#include <atomic>
#include <nlohmann/json.hpp>
#include <fcntl.h>  // For fcntl
#include <unistd.h> // For read, write
#include <errno.h>  // For errno

#include <random>
// you will need to install json

using json = nlohmann::json;
using namespace Sync; // Assuming Sync namespace is defined in one of the included headers

std::atomic<bool> running(true);

// Forward declaration of Socket to be used in Player
class Socket;

struct PlayerData
{
    int id;
    float posX;
};
std::unordered_map<int, PlayerData> playerData;
std::mutex playersMutex;

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

    virtual long ThreadMain()
    {
        std::string buffer; // Buffer to accumulate received data
        while (!shouldTerminate)
        {
            try
            {
                ByteArray receivedData;
                // Read operation...
                if (connectionSocket.Read(receivedData) > 0)
                {
                    buffer += receivedData.ToString(); // Append to buffer

                    size_t pos;
                    // Process all complete JSON messages in the buffer
                    while ((pos = buffer.find("\n")) != std::string::npos)
                    {
                        std::string message = buffer.substr(0, pos);
                        buffer.erase(0, pos + 1); // Remove processed message from buffer

                        try
                        {
                            json jsonData = json::parse(message); // Parse the JSON data
                            std::cout << "Received data: " << jsonData.dump() << std::endl;
                            // Process jsonData...
                        }
                        catch (json::parse_error &e)
                        {
                            std::cerr << "JSON parse error: " << e.what() << std::endl;
                            // Handle parse error (e.g., log it and continue)
                        }
                    }

                    // Optionally, send updated positions to all clients here or elsewhere
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Exception in client handler thread: " << e.what() << std::endl;
                shouldTerminate = true;
            }
            catch (...)
            {
                std::cerr << "Unknown exception in client handler thread." << std::endl;
                shouldTerminate = true;
            }
        }
        return 0;
    }

    void SendPlayerPositions()
    {
        std::lock_guard<std::mutex> lock(playersMutex);

        json playerPositions = json::array();
        for (const auto &player : players)
        {
            json playerInfo;
            playerInfo["id"] = player->id;
            playerInfo["positionX"] = player->positionX;
            playerPositions.push_back(playerInfo);
        }

        std::string positionMessage = playerPositions.dump() + "\n"; // Append newline character
        connectionSocket.Write(ByteArray(positionMessage));
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
                        std::cout << "New Client connected. Client thread starts..." << std::endl;
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
    std::cout << "Server is active. Listening on port 2005." << std::endl;

    SocketServer server(2005);
    ConnectionManager serverManager(server);

    std::cin.get(); // Wait for user input to terminate the server.

    std::cout << "Shutting down the server." << std::endl;
    server.Shutdown();

    running = false; // Set running to false to stop any running threads.

    return 0;
}