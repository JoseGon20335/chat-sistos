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

string ipClient()
{
    const char *dnsServer = "8.8.8.8";
    int dnsPort = 53;
    struct sockett serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        close(sock);
        return "";
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(dnsServer);
    serv.sin_port = htons(dnsPort);

    int err = connect(sock, (const struct sockaddr *)&serv, sizeof(serv));
    if (err < 0)
    {
        close(sock);
        return "";
    }

    struct sockett name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr *)&name, &namelen);

    char buffer[80];
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);

    if (p != NULL)
    {
        close(sock);
        string res = (string)buffer;
        return res;
    }
    else
    {
        close(sock);
        return "";
    }
}

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct sockett;
    struct server_info;
    int serverSocket;
    int serverPort;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(9000);

    int connectResult = connect(serverSocket, (struct sockaddr *)&server_info, sizeof(server_info));

    char buffer[2048] = {0};

    while (true)
    {
        buffer[2048] = {0};

        printf("Email: ");
        scanf("%s", buffer);

        if (strlen(buffer) != 0)
        {
            break;
        }
    }

    string ip = ipClient();
    string username = (string)buffer;

    printf("IP: %s\n", ip.c_str());
    printf("Email: %s\n", buffer);

    chat::UserRequest register_set;

    register_set.set_option(1);
    mut_new_user = register_set.mutable_newuser();
    mut_new_user->set_username(buffer);
    mut_new_user->set_ip(ip);

    string serialized;
    register_set.SerializeToString(&serialized);

    send(serverSocket, serialized.c_str(), serialized.length(), 0);

    int readResult = read(serverSocket, buffer, 2048);
    if (readResult < 0)
    {
        printf("Error reading from server\n");
        return 1;
    }

    chat::ServerResponse response;
    response.ParseFromString((string)buffer);

    if (response.code() == 200)
    {
        printf("200 OK: Todo bien\n");
    }
    else
    {
        printf("400 BAD: %s\n", response.servermessage().c_str());
        return 1;
    }

    bool *running = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *running = true;

    int pid = fork();

    if (pid == 0)
    {
        int sleepTime = 0;

        while (*running)
        {
            sleep(1);
            sleepTime++;

            if (sleepTime == 20)
            {
                chat::new_request heartbeat;
                heartbeat.set_option(5);

                string serialized;
                heartbeat.SerializeToString(&serialized);

                send(serverSocket, serialized.c_str(), serialized.length(), 0);
                sleepTime = 0;
            }
        }

        return 0;
    }
    else if (pid > 0)
    {
        int pid2 = fork();

        if (pid2 == 0)
        {
            while (*running)
            {
                int readResult = read(serverSocket, buffer, 2048);

                if (readResult < 0)
                {
                    printf("Error reading from server\n");
                    return 1;
                }

                chat::ServerResponse response;
                response.ParseFromString((string)buffer);

                if (response.code() == 200)
                {
                    if (response.option() == 2)
                    {
                        printf("Success: %s\n", response.servermessage().c_str());
                        for (int i = 0; i < response.mutable_connectedusers()->connectedusers_size(); i++)
                        {
                            printf("%s\n", response.mutable_connectedusers()->connectedusers(i).username().c_str());
                        }
                    }
                    else if (response.option() == 4)
                    {
                        if (response.has_message())
                        {
                            if (response.mutable_message()->message_type() == true)
                            {
                                printf("Public message from %s: %s\n", response.mutable_message()->sender().c_str(), response.mutable_message()->message().c_str());
                            }
                            else
                            {
                                printf("Private message from %s: %s\n", response.mutable_message()->sender().c_str(), response.mutable_message()->message().c_str());
                            }
                        }
                        else
                        {
                            printf("Success: %s\n", response.servermessage().c_str());
                        }
                    }
                }
                else
                {
                    printf("Error: %s\n", response.servermessage().c_str());
                }
            }

            return 0;
        }
        else if (pid2 > 0)
        {
            while (*running)
            {
                printf("1. Send private message\n");
                printf("2. Send public message\n");
                printf("3. Get list of users\n");
                printf("4. Exit\n");

                int choice;
                scanf("%d", &choice);

                switch (choice)
                {
                case 1:
                {
                    string recipient;
                    string message;

                    printf("Enter recipient: ");
                    scanf("%s", buffer);
                    recipient = (string)buffer;

                    printf("Enter message: ");
                    scanf("%s", buffer);
                    message = (string)buffer;

                    chat::new_request privateMessage;
                    privateMessage.set_option(4);
                    privateMessage.mutable_message()->set_recipient(recipient);
                    privateMessage.mutable_message()->set_message(message);
                    privateMessage.mutable_message()->set_message_type(false);
                    privateMessage.mutable_message()->set_sender(username);

                    string serialized;
                    privateMessage.SerializeToString(&serialized);

                    send(serverSocket, serialized.c_str(), serialized.length(), 0);

                    break;
                }

                case 2:
                {
                    string message;

                    printf("Enter message: ");
                    scanf("%s", buffer);
                    message = (string)buffer;

                    chat::new_request publicMessage;
                    publicMessage.set_option(4);
                    publicMessage.mutable_message()->set_message(message);
                    publicMessage.mutable_message()->set_message_type(true);
                    publicMessage.mutable_message()->set_sender(username);

                    string serialized;
                    publicMessage.SerializeToString(&serialized);

                    send(serverSocket, serialized.c_str(), serialized.length(), 0);

                    break;
                }

                case 3:
                {
                    chat::new_request userList;
                    userList.set_option(2);

                    userList.mutable_inforequest()->set_type_request(true);

                    string serialized;
                    userList.SerializeToString(&serialized);

                    send(serverSocket, serialized.c_str(), serialized.length(), 0);

                    break;
                }

                case 4:
                {
                    printf("Exiting... Please wait\n");
                    *running = false;
                    break;
                }

                default:
                {
                    printf("Invalid choice\n");
                    break;
                }
                }
            }
        }

        wait(NULL);
    }

    close(serverSocket);
    shutdown(serverSocket, SHUT_RDWR);
    google::protobuf::ShutdownProtobufLibrary();

    printf("Client is shutting down\n");
    munmap(&running, sizeof(bool));
    return 0;
}