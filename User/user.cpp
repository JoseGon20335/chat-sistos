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

string getIP()
{
    const char *dnsServer = "8.8.8.8";
    int dnsPort = 53;
    struct sockaddr_in serv;
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

    struct sockaddr_in name;
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

    struct sockaddr_in serverAddress;
    int serverSocket;
    int serverPort;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("Error creating socket\n");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
    {
        printf("Error converting address\n");
        return 1;
    }

    int connectResult = connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult < 0)
    {
        printf("Error connecting to server\n");
        return 1;
    }

    char buffer[1024] = {0};

    while (true)
    {
        buffer[0] = '\0'; // Changing buffer initialization

        printf("Enter email address: ");
        scanf("%s", buffer);

        bool hasAt = false;
        for (int i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '@')
            {
                hasAt = true;
                break;
            }
        }

        if (hasAt)
        {
            break;
        }
        else
        {
            printf("Invalid email address\n");
        }
    }

    string ip = getIP();
    string username = (string)buffer;

    printf("Client IP: %s\n", ip.c_str());
    printf("Email: %s\n", buffer);

    chat::UserRequest newRegister;

    newRegister.set_option(1);
    newRegister.mutable_newuser()->set_username(buffer);
    newRegister.mutable_newuser()->set_ip(ip);

    string serialized;
    newRegister.SerializeToString(&serialized);

    send(serverSocket, serialized.c_str(), serialized.length(), 0);

    int readResult = read(serverSocket, buffer, 1024);
    if (readResult < 0)
    {
        printf
