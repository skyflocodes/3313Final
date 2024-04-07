#include "SharedObject.h"
#include "Semaphore.h"
#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

class CommThread : public Thread
{
private:
    Socket theSocket;
public:
    CommThread(Socket const & p)
        : Thread(true),theSocket(p)
    {
        ;
    }
    int ThreadMain(void)
    {
        ByteArray bytes;
        std::cout << "Created a socket thread!" << std::endl;
        for(;;)
        {
            int read = theSocket.Read(bytes);
            if (read == -1)
            {
                std::cout << "Error in socket detected" << std::endl;
                break;
            }
            else if (read == 0)
            {
                std::cout << "Socket closed at remote end" << std::endl;
                break;
            }
            else
            {
                std::string theString = bytes.ToString();
                std::cout << "Received: " << theString << std::endl;
                bytes.v[0]='R';
                theSocket.Write(bytes);
            }
        }
        std::cout << "Thread is gracefully ending" << std::endl;
    }
};

class KillThread  : public Thread
{
private:
    SocketServer & theServer;
public:
    KillThread(SocketServer & t)
        : Thread(true),theServer(t)
    {
        ;
    }
    int ThreadMain(void)
    {
        std::cout << "Type 'quit' when you want to close the server" << std::endl;
        for (;;)
        {
            std::string s;
            std::cin >> s;
            if(s=="quit")
            {
                theServer.Shutdown();
                break;
            }
        }
    }
};

int main(void)
{
    std::cout << "I am a socket server" << std::endl;
    SocketServer theServer(2000);
    std::vector<CommThread *> threads;
    std::vector<Socket> playerConnections;

    Semaphore socketSem("Player",0,true);

    for(;;)
    {
        try
        {
            //wait for connection
            Socket newSocket = theServer.Accept();
            std::cout << "Received a socket connection!" << std::endl;
            //start thread
            threads.push_back(new CommThread(newSocket));
            playerConnections.push_back(newSocket);
        }
        catch(TerminationException e)
        {
            std::cout << "The socket server is no longer listening. Exiting now." << std::endl;
            break;
        }
        catch(std::string s)
        {
            std::cout << "thrown " << s << std::endl;
            break;
        }
        catch(...)
        {
            std::cout << "caught  unknown exception" << std::endl;
            break;
        }
    }
    std::cout << "End of main" << std::endl;
    for (int i=0;i<threads.size();i++)
        delete threads[i];
}

void asteroidsThreadMain(){

    int astroidSpeed =2;
    int counter = 0;
    
    std::chrono::seconds interval(2);

    while(true){
        std::this_thread::sleep_for(interval);
        counter=counter+2;
        if(counter%10==0) astroidSpeed++;


        //random position
        int position = randomInRange(0,800);

        ByteArray astroidData;

        //build ByteArray (position and speed)
        ByteArray astroidData = encodeAsteroids(position,astroidSpeed);

        //iterate through Player sockets, send astroidData
        socketSem.wait();

        for(const auto& connection: playerConnections){
            connection.Write(astroidData);
        }

    }
}

int randomInRange(int min, int max){

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distr(min,max);

    return distr(gen);
}

ByteArray encodeAsteroids(int pos, int speed) {
    ByteArray byteArray;

    //identifier for asteroid message
    byteArray.v.push_back('A');

    //2 bytes each for each number
    byteArray.v.push_back(static_cast<char>(pos >> 8)); // Most significant byte
    byteArray.v.push_back(static_cast<char>(pos));      // Least significant byte
    byteArray.v.push_back(static_cast<char>(speed >> 8)); 
    byteArray.v.push_back(static_cast<char>(speed));   

    return byteArray;
}

std::pair<int, int> decodeAsteroids(ByteArray payload) {

    //if asteroid message incorrect format
    if (payload.v.size() < 5) { 
        throw std::runtime_error("Not enough bytes to decode integers");
    }
    if (payload.v[0] != 'A') {
        throw std::runtime_error("Invalid identifier for asteroids data");
    }
    // Decode num1
    int num1 = (static_cast<unsigned char>(payload.v[1]) << 8) | static_cast<unsigned char>(payload.v[2]);

    // Decode num2
    int num2 = (static_cast<unsigned char>(payload.v[3]) << 8) | static_cast<unsigned char>(payload.v[4]);

    return std::make_pair(num1, num2);
}
