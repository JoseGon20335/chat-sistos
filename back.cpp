#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>
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

cliente clienteOb[100] = {};

void *clienteHandler(void *arg)
{
    pthread_t thisThread = pthread_self();
    int slot = -1;
    int clientSocket = *(int *)arg;
    printf("clientSocket %d\n", clientSocket);

    char buffer[2048] = {0};
    int readResult;

    bool loop = true;
    bool temp = true;
    while (loop)
    {
        bool heartbeat = false;
        int pid = fork();

        if (pid == 0)
        {
            int timeout = 0;

            while (temp)
            {
                sleep(10);
                timeout += 10;
                printf("Timeout: %d\n", timeout);

                if (timeout >= 60)
                {
                    close(clientSocket);
                    heartbeat = true;
                    temp = false;
                }
            }
            temp = true;
        }
        else if (pid > 0)
        {
            readResult = read(clientSocket, buffer, 2048);
            kill(pid, SIGKILL);
            wait(NULL);
        }

        if (heartbeat)
        {
            printf("Inactividad timeout\n");
            if (slot != -1)
            {
                clienteOb[slot].username = "";
                clienteOb[slot].ip = "";
                clienteOb[slot].socket = 0;
                clienteOb[slot].status = 0;
            }
            loop = false;
        }

        if (readResult < 0)
        {
            printf("ERROR");
            loop = false;
        }
        else if (readResult == 0)
        {
            printf("Cliente desconectado\n");
            if (slot != -1)
            {
                clienteOb[slot].username = "";
                clienteOb[slot].ip = "";
                clienteOb[slot].socket = 0;
                clienteOb[slot].status = 0;
            }
            loop = false;
        }
        else
        {
            chat::UserRequest user_request;
            user_request.ParseFromString((string)buffer);

            if (user_request.option() == 1)
            {
                // User registration
                printf("User: %s\n se intentara registrar\n", user_request.mutable_newuser()->username().c_str());
                printf("Con la ip: %s\n", user_request.mutable_newuser()->ip().c_str());

                chat::ServerResponse server_response;
                server_response.set_option(1);
                server_response.set_code(400);
                server_response.set_servermessage("Error registering user");

                bool flagExist = false;

                for (int i = 0; i < 100; i++)
                {
                    if (clienteOb[i].ip == user_request.mutable_newuser()->ip())
                    {
                        flagExist = true;
                    }
                }

                if (flagExist)
                {
                    printf("Esta IP %s ya tiene una cuenta unida\n", user_request.mutable_newuser()->ip().c_str());
                    server_response.set_servermessage("IP ya registrada");
                }
                else
                {
                    int slot = -1;
                    for (int i = 0; i < 100; i++)
                    {
                        if (clienteOb[i].username == "")
                        {
                            slot = i;
                        }
                    }

                    if (slot == -1)
                    {
                        printf("No empty slots\n");
                        loop = false;
                    }

                    clienteOb[slot].username = user_request.mutable_newuser()->username();
                    clienteOb[slot].ip = user_request.mutable_newuser()->ip();
                    clienteOb[slot].socket = clientSocket;
                    clienteOb[slot].status = 1;

                    server_response.set_code(200);
                    server_response.set_servermessage("Registro exitoso");

                    printf("Usario %s\n", clienteOb[slot].username.c_str());
                    printf("con IP %s\n", clienteOb[slot].ip.c_str());
                    printf("se registro correctamente\n");
                }

                string responseString;
                server_response.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);
            }
            else if (user_request.option() == 2)
            {
                if (user_request.mutable_inforequest()->type_request() == true)
                {
                    // All users
                    printf("User: %s pide la lista de usuarios\n", clienteOb[slot].username.c_str());

                    chat::ServerResponse server_response;
                    server_response.set_option(2);
                    server_response.set_code(200);
                    server_response.set_servermessage("Lista usuarios");

                    for (int i = 0; i < 100; i++)
                    {
                        if (clienteOb[i].username != "")
                        {
                            chat::UserInfo *user_info = server_response.mutable_connectedusers()->add_connectedusers();
                            user_info->set_username(clienteOb[i].username);
                            user_info->set_ip(clienteOb[i].ip);
                            user_info->set_status(clienteOb[i].status);
                        }
                    }

                    string responseString;
                    server_response.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);
                }
                else
                {
                    // Single user
                    printf("User %s\n", clienteOb[slot].username.c_str());
                    printf("Trata de acceder a %s\n", user_request.mutable_inforequest()->user().c_str());

                    chat::ServerResponse server_response;
                    server_response.set_option(2);
                    server_response.set_code(400);
                    server_response.set_servermessage("Usuario no ubicado");

                    for (int i = 0; i < 100; i++)
                    {
                        if (clienteOb[i].username == user_request.mutable_inforequest()->user())
                        {
                            chat::UserInfo *user_info = server_response.mutable_connectedusers()->add_connectedusers();
                            user_info->set_username(clienteOb[i].username);
                            user_info->set_ip(clienteOb[i].ip);
                            user_info->set_status(clienteOb[i].status);

                            server_response.set_code(200);
                            server_response.set_servermessage("Usuario no ubicado");
                            loop = false;
                        }
                    }

                    string responseString;
                    server_response.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);
                }
            }
            else if (user_request.option() == 3)
            {
                for (int i = 0; i < 100; i++)
                {
                    if (clienteOb[i].username == user_request.mutable_status()->username())
                    {
                        clienteOb[i].status = user_request.mutable_status()->newstatus();
                        loop = false;
                    }
                }
            }
            else if (user_request.option() == 4)
            {
                string mensaje = user_request.mutable_message()->message();
                string envia = user_request.mutable_message()->sender();
                string recibe;
                bool online = false;
                int recipientSlot = -1;

                if (user_request.mutable_message()->message_type() == true)
                {
                    recibe = "public";
                    chat::ServerResponse server_response;
                    server_response.set_option(4);
                    server_response.set_code(200);
                    server_response.set_servermessage("Mensaje enviado");

                    string responseString;
                    server_response.SerializeToString(&responseString);

                    send(clientSocket, responseString.c_str(), responseString.length(), 0);

                    chat::ServerResponse mensaje_enviado;
                    mensaje_enviado.set_option(4);
                    mensaje_enviado.set_code(200);
                    mensaje_enviado.set_servermessage("Nuevo mensaje");
                    mensaje_enviado.mutable_message()->set_message(mensaje);
                    mensaje_enviado.mutable_message()->set_message_type(true);
                    for (int i = 0; i < 100; i++)
                    {
                        if (clienteOb[i].username != envia && clienteOb[i].status != 0)
                        {
                            send(clienteOb[i].socket, responseString.c_str(), responseString.length(), 0);
                        }
                    }
                }
                else
                {
                    recibe = user_request.mutable_message()->recipient();
                    for (int i = 0; i < 100; i++)
                    {
                        if (clienteOb[i].username == recibe && clienteOb[i].status != 0)
                        {
                            online = true;
                            recipientSlot = i;
                            loop = false;
                        }
                    }

                    if (online)
                    {
                        chat::ServerResponse server_response;
                        server_response.set_option(4);
                        server_response.set_code(200);
                        server_response.set_servermessage("Mensaje enviado");

                        string responseString;
                        server_response.SerializeToString(&responseString);

                        send(clientSocket, responseString.c_str(), responseString.length(), 0);

                        chat::ServerResponse mensaje_enviado;
                        mensaje_enviado.set_option(4);
                        mensaje_enviado.set_code(200);
                        mensaje_enviado.set_servermessage("Nuevo mensaje");
                        mensaje_enviado.mutable_message()->set_message(mensaje);
                        mensaje_enviado.mutable_message()->set_message_type(false);
                        mensaje_enviado.mutable_message()->set_sender(envia);
                        mensaje_enviado.mutable_message()->set_recipient(recibe);

                        for (int i = 0; i < 100; i++)
                        {
                            if (clienteOb[i].username == envia)
                            {
                                send(clienteOb[i].socket, responseString.c_str(), responseString.length(), 0);
                                loop = false;
                            }
                        }
                    }
                    else
                    {
                        printf("Thread %lu: User %s is offline\n", thisThread, recibe.c_str());

                        chat::ServerResponse server_response;
                        server_response.set_option(4);
                        server_response.set_code(400);
                        server_response.set_servermessage("User is offline");

                        string responseString;
                        server_response.SerializeToString(&responseString);

                        send(clientSocket, responseString.c_str(), responseString.length(), 0);
                    }
                }

                for (int i = 0; i < 100; i++)
                {
                    if (clienteOb[i].username == recibe && clienteOb[i].status != 0)
                    {
                        online = true;
                        recipientSlot = i;
                        loop = false;
                    }
                }
            }
            else if (user_request.option() == 5)
            {
                // Heartbeat
                printf("Heartbeat\n");

                chat::ServerResponse server_response;
                server_response.set_option(5);
                server_response.set_code(200);
                server_response.set_servermessage("Heartbeat response");

                string responseString;
                server_response.SerializeToString(&responseString);

                send(clientSocket, responseString.c_str(), responseString.length(), 0);
            }
            else
            {
                // Unknown request
                printf("Thread %lu: Unknown request\n", thisThread);
            }
        }
    }

    pthread_exit(NULL);
}

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in serverAddress;
    int server;
    int port;

    server = socket(AF_INET, SOCK_STREAM, 0);

    port = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &port, sizeof(port));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    int bindResult = bind(server, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    while (true)
    {
        printf("Conectando con el servidor\n");

        int size = sizeof(serverAddress);
        int socket = accept(server, (struct sockaddr *)&serverAddress, (socklen_t *)&size);

        if (socket < 0)
        {
            printf("Error");
            return 1;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, &clienteHandler, (void *)&socket);
    }

    close(server);
    shutdown(server, SHUT_RDWR);

    printf("Server is shutting down\n");
    return 0;
}