#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "project.pb.h"

using std::string;

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("IP: %s\n", argv[1]);
        printf("PORT: %s\n", argv[2]);
        return 1;
    }
    string serverIP = (string)argv[1];
    int InputPort = atoi(argv[2]);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in server_info;
    int server;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        printf("Socket fail\n");
        return 1;
    }

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(InputPort);
    int connectResult;
    printf("Trying to convert adress\n");
    if (inet_pton(AF_INET, serverIP.c_str(), &server_info.sin_addr) <= 0)
    {
        printf("Error\n");
        return 1;
    }
    else
    {
        connectResult = connect(server, (struct sockaddr *)&server_info, sizeof(server_info));
        printf("Connectingto the server\n");
        if (connectResult < 0)
        {
            printf("Error\n");
            return 1;
        }
    }

    char buffer[2048] = {0};

    bool flagFillEnmail = true;

    while (flagFillEnmail)
    {
        buffer[2048] = {0};

        printf("Email: ");
        scanf("%s", buffer);

        if (strlen(buffer) != 0)
        {
            flagFillEnmail = false;
        }
    }

    string ip;
    const char *dnsServer = "8.8.8.8";
    int dnsPort = 53;
    struct sockaddr_in serverIp;
    int socketInt = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketInt < 0)
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    memset(&serverIp, 0, sizeof(serverIp));
    serverIp.sin_family = AF_INET;
    serverIp.sin_addr.s_addr = inet_addr(dnsServer);
    serverIp.sin_port = htons(dnsPort);

    int err = connect(socketInt, (const struct sockaddr *)&serverIp, sizeof(serverIp));
    if (err < 0)
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(socketInt, (struct sockaddr *)&name, &namelen);

    char ipCharB[80];
    const char *p = inet_ntop(AF_INET, &name.sin_addr, ipCharB, 80);

    if (p != NULL)
    {
        close(socketInt);
        ip = (string)ipCharB;
    }
    else
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    string username = (string)buffer;

    printf("IP: %s\n", ip.c_str());
    printf("Email: %s\n", buffer);

    chat::UserRequest register_set;

    register_set.set_option(1);
    register_set.mutable_newuser()->set_username(buffer);
    register_set.mutable_newuser()->set_ip(ip);

    string serialized;
    register_set.SerializeToString(&serialized);

    send(server, serialized.c_str(), serialized.length(), 0);
    printf("Sending register request\n");
    int readResult = read(server, buffer, 2048);
    printf("Reading server\n");
    if (readResult < 0)
    {
        printf("Error\n");
        return 1;
    }

    chat::ServerResponse server_responce;
    server_responce.ParseFromString((string)buffer);

    if (server_responce.code() == 200)
    {
        printf("200 OK: Todo bien\n");
    }
    else
    {
        printf("400 BAD: %s\n", server_responce.servermessage().c_str());
        return 1;
    }

    int flags = fcntl(server, F_GETFL, 0);
    fcntl(server, F_SETFL, flags | O_NONBLOCK);

    bool *flag = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *flag = true;

    int pid = fork();

    if (pid == 0)
    {
        int out = 0;

        while (*flag)
        {
            sleep(1);
            out++;
            if (out == 60)
            {
                chat::UserRequest heart_beat;
                heart_beat.set_option(5);

                string serialized;
                heart_beat.SerializeToString(&serialized);

                send(server, serialized.c_str(), serialized.length(), 0);
                out = 0;
            }
        }

        return 0;
    }
    else if (pid > 0)
    {
        int pid2 = fork();

        if (pid2 == 0)
        {
            while (*flag)
            {
                int readResult = NULL;

                while (readResult == NULL && *flag)
                {
                    readResult = read(server, buffer, 2048);
                }

                if (readResult < 0)
                {
                    printf("Error\n");
                    return 1;
                }

                chat::ServerResponse server_responce;
                server_responce.ParseFromString((string)buffer);

                if (server_responce.code() == 200)
                {
                    if (server_responce.option() == 2)
                    {
                        printf("Success");
                        for (int i = 0; i < server_responce.mutable_connectedusers()->connectedusers_size(); i++)
                        {
                            printf("%s\n", server_responce.mutable_connectedusers()->connectedusers(i).username().c_str());
                        }
                    }
                    else if (server_responce.option() == 4)
                    {
                        if (server_responce.has_message())
                        {
                            if (server_responce.mutable_message()->message_type() == true)
                            {
                                printf("PUBLICO: %s: %s\n", server_responce.mutable_message()->sender().c_str(), server_responce.mutable_message()->message().c_str());
                            }
                            else
                            {
                                printf("PRIVADA %s: %s\n", server_responce.mutable_message()->sender().c_str(), server_responce.mutable_message()->message().c_str());
                            }
                        }
                        else
                        {
                            printf("Success");
                        }
                    }
                }
                else
                {
                    printf("Error 400");
                }
            }

            return 0;
        }
        else if (pid2 > 0)
        {
            while (*flag)
            {
                printf("1. Private message\n");
                printf("2. Public message\n");
                printf("3. Status active\n");
                printf("4. Status offline\n");
                printf("5. Status absent\n");
                printf("6. User list\n");
                printf("7. Specific user info\n");
                printf("9. Log out\n");

                int temp;
                scanf("%d", &temp);

                if (temp == 1)// privado
                {
                    string destino, mensaje;

                    printf("Destinatario:\n");
                    scanf("%s", buffer);
                    destino = (string)buffer;

                    printf("Mensaje:");
                    scanf("%s", buffer);
                    mensaje = (string)buffer;

                    chat::UserRequest user_request;
                    user_request.set_option(4);
                    user_request.mutable_message()->set_recipient(destino);
                    user_request.mutable_message()->set_message(mensaje);
                    user_request.mutable_message()->set_message_type(false);
                    user_request.mutable_message()->set_sender(username);

                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 2)// publico
                {
                    string mensaje;

                    printf("Enter mensaje: ");
                    scanf("%s", buffer);
                    mensaje = (string)buffer;

                    chat::UserRequest user_request;
                    user_request.set_option(4);
                    user_request.mutable_message()->set_message(mensaje);
                    user_request.mutable_message()->set_message_type(true);
                    user_request.mutable_message()->set_sender(username);

                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 3)// activo
                {
                    int statusCode = 1;

                    chat::UserRequest statusChange;
                    statusChange.set_option(3);
                    statusChange.mutable_status()->set_newstatus(statusCode);
                    statusChange.mutable_status()->set_username(username);

                    string serialized;
                    statusChange.SerializeToString(&serialized);

                    send(serverSocket, serialized.c_str(), serialized.length(), 0);

                }
                else if (temp == 4)// desconectado
                {
                    int statusCode = 2;

                    chat::UserRequest statusChange;
                    statusChange.set_option(3);
                    statusChange.mutable_status()->set_newstatus(statusCode);
                    statusChange.mutable_status()->set_username(username);

                    string serialized;
                    statusChange.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 5)// ausente
                {
                    int statusCode = 3;

                    chat::UserRequest statusChange;
                    statusChange.set_option(3);
                    statusChange.mutable_status()->set_newstatus(statusCode);
                    statusChange.mutable_status()->set_username(username);

                    string serialized;
                    statusChange.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 6)// lista de usuarios
                {
                    chat::UserRequest user_request;
                    user_request.set_option(1);
                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 7)// info de usuario
                {
                    /*
                    string user;

                    printf("who?:");
                    scanf("%s", buffer);
                    user = (string)buffer;

                    chat::UserRequest user_request;
                    user_request.set_option(2);
                    user_request.mutable_userinfo()->set_username(user);

                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                    */
                   
                }
                else if (temp == 9)// salir
                {
                    *flag = false;
                    chat::UserRequest user_request;
                    user_request.set_option(5);
                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else
                {
                    printf("No entre las opciones\n");
                }
            }
            wait(NULL);
        }
        wait(NULL);
    }

    close(server);
    shutdown(server, SHUT_RDWR);
    google::protobuf::ShutdownProtobufLibrary();

    printf("Client is shutting down\n");
    munmap(&flag, sizeof(bool));
    return 0;
}
