#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "project.pb.h"

using std::string;

struct cliente
{
    string username;
    string ip;
    int socket;
    int status;
};

cliente allClients[10] = {};

void *handler(void *arg)
{
    pthread_t thisThread = pthread_self();
    int slot = -1;
    int socketInt = *(int *)arg;

    printf("Un cliete conectado\n");

    char buffer[2048] = {0};
    int readPid;
    bool flagLive = true;
    bool *reading = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    bool *dieT = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int flags = fcntl(socketInt, F_GETFL, 0);
    fcntl(socketInt, F_SETFL, flags | O_NONBLOCK);

    while (flagLive)
    {
        memset(buffer, 0, 2048);
        *dieT = false;
        *reading = true;
        int pid = fork();
        int timeInactive = 0;

        if (pid == 0)
        {
            timeInactive = 0;
            while (*reading)
            {
                sleep(1);
                timeInactive += 1;

                if (timeInactive >= 180)
                {
                    close(socketInt);
                    *dieT = true;
                    break;
                }
            }
            return 0;
        }
        else if (pid > 0)
        {
            readPid = -1;

            while (readPid == -1 && !*dieT)
            {
                readPid = read(socketInt, buffer, 2048);
            }

            *reading = false;
            wait(NULL);
        }

        if (*dieT)
        {
            if (slot != -1)
            {
                allClients[slot].status = 2;
            }

            break;
        }

        if (readPid < 0)
        {
            printf("Error: cant read pid client socket\n");
            break;
        }
        else if (readPid == 0)
        {
            printf("Client disconnected\n");

            if (slot != -1)
            {
                allClients[slot].username = "";
                allClients[slot].ip = "";
                allClients[slot].socket = 0;
                allClients[slot].status = 0;
            }
            break;
        }
        else
        {
            chat::UserRequest newRequest;
            newRequest.ParseFromArray(buffer, 2048);

            if (newRequest.option() == 1)
            {
                printf("New Request from user: REGISTER\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());

                chat::ServerResponse newResponse;
                newResponse.set_option(1);
                newResponse.set_code(400);
                newResponse.set_servermessage("Error registering user");

                bool canRegister = true;

                for (int i = 0; i < 10; i++)
                {
                    if (strcmp(allClients[i].username.c_str(), newRequest.mutable_newuser()->username().c_str()) == 0)
                    {
                        printf("We alredy have a user whit the username: %s\n", newRequest.mutable_newuser()->username().c_str());
                        newResponse.set_servermessage("IP already register");
                        canRegister = false;
                    }
                }

                if (canRegister)
                {
                    bool flagExist = false;
                    for (int i = 0; i < 10; i++)
                    {
                        printf("Check Slot: %d\n", i);
                        if (strcmp(allClients[i].username.c_str(), "") == 0)
                        {
                            printf("New client in server client slot number: %d\n", i);
                            slot = i;
                            flagExist = true;
                            i = 10;
                        }
                    }

                    if (!flagExist)
                    {
                        printf("Unable to set new client due to dont have enoght slot, sorry :(\n", thisThread);
                        flagLive = false;
                    }
                    else
                    {
                        allClients[slot].username = newRequest.mutable_newuser()->username();
                        allClients[slot].ip = newRequest.mutable_newuser()->ip();
                        allClients[slot].socket = socketInt;
                        allClients[slot].status = 1;
                        printf("New client in server client slot number: %d\n", slot);
                        printf("Register a new user sucesfully:\n");
                        printf("Username: %s\n", allClients[slot].username.c_str());
                        printf("IP: %s\n", allClients[slot].ip.c_str());
                    }
                    newResponse.set_code(200);
                    newResponse.set_servermessage("User registered sucesfully");
                }

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(socketInt, responseString.c_str(), responseString.length(), 0);
            }
            else if (newRequest.option() == 2)
            {
                bool type_request = newRequest.mutable_inforequest()->type_request();
                printf("%d", type_request);

                if (type_request == false)
                {
                    printf("New Request from user: GET SINGLE USER\n");
                    printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                    printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());

                    std::string dataTemp = newRequest.inforequest().user();

                    chat::ServerResponse newResponse;
                    newResponse.set_option(2);
                    newResponse.set_code(200);
                    bool flagGetUsers = false;
                    for (int i = 0; i < 10; i++)
                    {
                        if (strcmp(allClients[i].username.c_str(), dataTemp.c_str()) == 0)
                        {
                            flagGetUsers = true;
                            newResponse.mutable_userinforesponse()->set_username(allClients[i].username);
                            newResponse.mutable_userinforesponse()->set_ip(allClients[i].ip);
                            newResponse.mutable_userinforesponse()->set_status(allClients[i].status);
                        }
                    }

                    if (flagGetUsers)
                    {
                        newResponse.set_servermessage("Users found");
                    }
                    else
                    {
                        newResponse.set_servermessage("No users found");
                        newResponse.set_code(400);
                    }

                    string responseString;
                    newResponse.SerializeToString(&responseString);
                    send(socketInt, responseString.c_str(), responseString.length(), 0);
                }
                else if (type_request == true)
                {
                    printf("New Request from user: GET USERS\n");
                    printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                    printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());

                    chat::ServerResponse newResponse;
                    newResponse.set_option(2);
                    newResponse.set_code(200);
                    bool flagGetUsers = false;
                    for (int i = 0; i < 10; i++)
                    {
                        if (allClients[i].username.empty() == false)
                        {
                            flagGetUsers = true;
                            chat::UserInfo *newUser = newResponse.mutable_connectedusers()->add_connectedusers();
                            newUser->set_username(allClients[i].username);
                            newUser->set_ip(allClients[i].ip);
                            newUser->set_status(allClients[i].status);
                        }
                    }

                    if (flagGetUsers)
                    {
                        newResponse.set_servermessage("Users found");
                    }
                    else
                    {
                        newResponse.set_servermessage("No users found");
                        newResponse.set_code(400);
                    }

                    string responseString;
                    newResponse.SerializeToString(&responseString);
                    send(socketInt, responseString.c_str(), responseString.length(), 0);
                }
            }
            else if (newRequest.option() == 3)
            {
                printf("New Request from user: CHANGE STATUS\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());
                chat::ServerResponse newResponse;
                newResponse.set_option(3);
                newResponse.set_code(400);
                newResponse.set_servermessage("Status failed to change");
                std::string dataTemp = newRequest.inforequest().user();
                for (int i = 0; i < 10; i++)
                {
                    if (strcmp(allClients[i].username.c_str(), dataTemp.c_str()) == 0)
                    {
                        allClients[i].status = newRequest.mutable_status()->newstatus();
                        printf("Thread %lu: User %s changed status to %d\n", thisThread, newRequest.mutable_status()->username().c_str(), newRequest.mutable_status()->newstatus());
                        newResponse.set_code(200);
                        newResponse.set_servermessage("Status changed");
                        flagLive = false;
                    }
                }

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(socketInt, responseString.c_str(), responseString.length(), 0);
            }
            else if (newRequest.option() == 4 && newRequest.mutable_message()->message_type() == true)
            {
                printf("New Request from user: NEW MESSAGE ALL\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());
                printf("Message:%s\n", newRequest.mutable_message()->message());

                chat::ServerResponse newResponse;
                newResponse.set_option(4);
                newResponse.set_code(200);
                newResponse.set_servermessage("Sendin message");
                string responseString;
                newResponse.SerializeToString(&responseString);

                send(socketInt, responseString.c_str(), responseString.length(), 0);
                memset(buffer, 0, sizeof(buffer));

                chat::ServerResponse sentMessage;
                sentMessage.set_option(4);
                sentMessage.set_code(200);
                sentMessage.set_servermessage("New message");
                sentMessage.mutable_message()->CopyFrom(newRequest.message());
                string sentMsg;
                sentMessage.SerializeToString(&sentMsg);
                string sender = newRequest.mutable_message()->sender();

                for (int i = 0; i < 10; i++)
                {
                    bool isSameUser = false;
                    bool isUserOnline = false;
                    if (allClients[i].username != sender)
                    {
                        isSameUser = true;
                    }
                    if (allClients[i].status != 0)
                    {
                        isUserOnline = true;
                    }
                    if (!isSameUser && isUserOnline)
                    {
                        send(allClients[i].socket, sentMsg.c_str(), sentMsg.size(), 0);
                        newRequest.mutable_message()->sender();
                    }
                }
            }
            else if (newRequest.option() == 4 && newRequest.mutable_message()->message_type() == false)
            {
                printf("New Request from user: NEW MESSAGE PRIVATE\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());
                printf("Message:%s\n", newRequest.mutable_message()->message());
                printf("Para:%s\n", newRequest.mutable_message()->recipient());

                chat::ServerResponse newResponse;
                newResponse.set_option(4);
                newResponse.set_code(200);
                newResponse.set_servermessage("Message sent");

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(socketInt, responseString.c_str(), responseString.length(), 0);

                memset(buffer, 0, sizeof(buffer));

                chat::ServerResponse sentMessage;
                sentMessage.set_option(4);
                sentMessage.set_code(200);
                sentMessage.set_servermessage("New message");
                sentMessage.mutable_message()->CopyFrom(newRequest.message());

                string sentMsg;
                sentMessage.SerializeToString(&sentMsg);

                for (int i = 0; i < 100; i++)
                {
                    if (strcmp(allClients[i].username.c_str(), newRequest.mutable_message()->recipient().c_str()) == 0)
                    {
                        if (allClients[i].status != 0)
                        {
                            send(allClients[i].socket, sentMsg.c_str(), sentMsg.size(), 0);
                            flagLive = false;
                        }
                        else
                        {
                            printf("Request faile, the destinatary is offline\n");
                            printf("Para:%s\n", newRequest.mutable_message()->recipient());

                            chat::ServerResponse newResponse;
                            newResponse.set_option(4);
                            newResponse.set_code(400);
                            newResponse.set_servermessage("User offline");
                            string responseString;
                            newResponse.SerializeToString(&responseString);

                            send(socketInt, responseString.c_str(), responseString.length(), 0);
                        }
                    }
                }
            }
            else if (newRequest.option() == 5)
            {
                // Heartbeat
                printf("New Request from user: HEARTBEAT\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());

                chat::ServerResponse newResponse;
                newResponse.set_option(5);
                newResponse.set_code(200);
                newResponse.set_servermessage("Heartbeat ok");

                string responseString;
                newResponse.SerializeToString(&responseString);

                send(socketInt, responseString.c_str(), responseString.length(), 0);
            }
            else
            {
                printf("New Request from user: INVALID OPTION\n");
                printf("User:%s\n", newRequest.mutable_newuser()->username().c_str());
                printf("IP:%s\n", newRequest.mutable_newuser()->ip().c_str());
            }
        }
    }

    munmap(&reading, sizeof(bool));
    munmap(&dieT, sizeof(bool));

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int usagePort = atoi(argv[1]);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in serverAddress;
    int socket_server;
    int port;

    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    port = setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &port, sizeof(port));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(usagePort);

    int binds = bind(socket_server, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    printf("Check socket\n");
    if (socket_server < 0)
    {
        printf("Error\n");
        return 1;
    }
    printf("Check port\n");
    if (port < 0)
    {
        printf("Error\n");
        return 1;
    }
    printf("Check bind\n");
    if (binds < 0)
    {
        printf("Error\n");
        return 1;
    }
    printf("Listen socket\n");
    if (listen(socket_server, 5) < 0)
    {
        printf("Error\n");
        return 1;
    }

    while (true)
    {
        printf("-------------------\n");
        printf("Servidor conectado!\n");
        printf("-------------------\n");

        int serverSize = sizeof(serverAddress);
        int socketAccept = accept(socket_server, (struct sockaddr *)&serverAddress, (socklen_t *)&serverSize);

        if (socketAccept < 0)
        {
            printf("Error socket");
            return 1;
        }
        pthread_t thread;
        pthread_create(&thread, NULL, &handler, (void *)&socketAccept);
    }

    close(socket_server);
    shutdown(socket_server, SHUT_RDWR);
    return 0;
}
