#include <iostream>
#include <vector>
#include <mutex>
#include "thread.h" // Assuming Thread class is defined here
#include "socketserver.h" // Assuming SocketServer class is defined here

// Forward declaration for ByteArray class
class ByteArray;

using namespace Sync;

class Player {
public:
    int id;
    int positionX = 0;

    void MoveLeft() { positionX -= 1; }
    void MoveRight() { positionX += 1; }
    int GetPositionX() const { return positionX; }
};

std::vector<Player*> players;
std::mutex playersMutex;

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
                if (move == "left" && !players.empty())
                    players[0]->MoveLeft();
                else if (move == "right" && !players.empty())
                    players[0]->MoveRight();
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

    std::cin.get();

    std::cout << "Shutting down the server." << std::endl;
    server.Shutdown();

    std::lock_guard<std::mutex> lock(playersMutex);
    for (auto* player : players) {
        delete player;
    }
    players.clear();

    return 0;
}
