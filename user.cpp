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
#include "project.pb.h"

using std::string;

int main(int argc, char **argv)
{
    string serverIP = (string)argv[1];
    int serverPort = atoi(argv[2]);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockaddr_in server_info;
    int server;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        printf("Error creating socket\n");
        return 1;
    }

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(8080);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0)
    {
        printf("Error converting address\n");
        return 1;
    }

    int connectResult = connect(server, (struct sockaddr *)&server_info, sizeof(server_info));
    if (connectResult < 0)
    {
        printf("Error connecting to server\n");
        return 1;
    }

    char buffer[2048] = {0};
    char em[2048];

    while (true)
    {
        buffer[2048] = {0};

        printf("Email: ");
        scanf("%s", buffer);

        if (strlen(buffer) != 0)
        {
            em = buffer;
            break;
        }
    }

    string ip;
    const char *dnsServer = "8.8.8.8";
    int dnsPort = 53;

    int socketInt = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv
    {
    };
    if (socketInt < 0)
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(dnsServer);
    serv.sin_port = htons(dnsPort);

    int err = connect(socketInt, (const struct sockaddr *)&serv, sizeof(serv));
    if (err < 0)
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(socketInt, (struct sockaddr *)&name, &namelen);

    buffer[80];
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);

    if (p != NULL)
    {
        close(socketInt);
        string res = (string)buffer;
    }
    else
    {
        close(socketInt);
        printf("Error con IP\n");
        return 1;
    }

    int connectResult = connect(err, (struct sockaddr *)&err, sizeof(err));
    if (connectResult < 0)
    {
        printf("Error connecting to server\n");
        return 1;
    }

    string username = (string)em;

    printf("IP: %s\n", ip.c_str());
    printf("Email: %s\n", em);

    chat::UserRequest register_set;

    register_set.set_option(1);
    register_set.mutable_newuser()->set_username(em);
    register_set.mutable_newuser()->set_ip(ip);

    string serialized;
    register_set.SerializeToString(&serialized);

    send(server, serialized.c_str(), serialized.length(), 0);

    int readResult = read(server, buffer, 2048);
    if (readResult < 0)
    {
        printf("Error reading from server\n");
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

            if (out == 20)
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
                int readResult = read(server, buffer, 2048);

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
                printf("1. Send private mensaje\n");
                printf("2. Send public mensaje\n");
                printf("3. Get list of users\n");
                printf("4. Exit\n");

                int temp;
                scanf("%d", &temp);

                if (temp == 1)
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
                else if (temp == 2)
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
                else if (temp == 3)
                {
                    chat::UserRequest user_request;
                    user_request.set_option(2);
                    user_request.mutable_inforequest()->set_type_request(true);
                    string serialized;
                    user_request.SerializeToString(&serialized);

                    send(server, serialized.c_str(), serialized.length(), 0);
                }
                else if (temp == 4)
                {
                    printf("Bye bye...\n");
                    *flag = false;
                }
                else
                {
                    printf("No entre las opciones\n");
                }
            }
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
