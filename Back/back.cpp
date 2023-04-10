#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include "project.pb.h"

using std::string;

struct Client
{
    string username;
    string ip;
    int socket;
    int status;
};

Client clients[100] = {};

int getFirstEmptySlot()
{
    for (int i = 0; i < 100; i++)
    {
        if (clients[i].username == "")
        {
            return i;
        }
    }

    return -1;
}

bool checkIfUserExists(string ip)
{
    for (int i = 0; i < 100; i++)
    {
        if (clients[i].ip == ip)
        {
            return true;
        }
    }

    return false;
}

void *clientHandler(void *arg)
{
    pthread_t thisThread = pthread_self();
    int clientSlot = -1;
    int clientSocket = *(int *)arg;

    printf("Thread %lu is handling client\n", thisThread);

    char buffer[1024] = {0};
    int readResult;

    while (true)
    {
        bool noHeartbeat = false;
        int pid = fork();

        if (pid == 0)
        {
            int timeInactive = 0;

            while (true)
            {
                sleep(10);
                timeInactive += 10;
                printf("Thread %lu: No heartbeat received - %d seconds\n", thisThread, timeInactive);

                if (timeInactive >= 60)
                {
                    close(clientSocket);
                    noHeartbeat = true;
                    break;
                }
            }
        }
        else if (pid > 0)
        {
            readResult = read(clientSocket, buffer, 1024);
            kill(pid, SIGKILL);
            wait(NULL);
        }

        if (noHeartbeat)
        {
            printf("Thread %lu: Client disconnected due to inactivity\n", thisThread);

            if (clientSlot != -1)
            {
                clients[clientSlot].username = "";
                clients[clientSlot].ip = "";
                clients[clientSlot].socket = 0;
                clients[clientSlot].status = 0;
            }

            break;
        }

        if (readResult < 0)
        {
            printf("Thread %lu: Error reading from socket\n", thisThread);
            break;
        }
        else if (readResult == 0)
        {
            printf("Thread %lu: Client disconnected\n", thisThread);

            if (clientSlot != -1)
            {
                clients[clientSlot].username = "";
                clients[clientSlot].ip = "";
                clients[clientSlot].socket = 0;
                clients[clientSlot].status = 0;
            }

            break;
        }
        else
        {
            chat::UserRequest newRequest;
            newRequest.ParseFromArray(buffer, readResult);

            if (newRequest.option() == 1)
            {
                // User registration
                printf("Thread %lu: User %s wants to register with IP %s\n", thisThread, newRequest.mutable_newuser()->username().c_str(), newRequest.mutable_newuser()->ip().c_str());

                chat::ServerResponse newResponse;
                newResponse.set_option(1);
                newResponse.set_code(400);
                newResponse.set_servermessage("Error registering user");

                if (checkIfUserExists(newRequest.mutable_newuser()->ip()))
                {
                    printf("Thread %lu: User with IP %s already exists\n", thisThread, new
