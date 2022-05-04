/* doc
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    char buffer[257];
    int sock, newSock;
    socklen_t serverlen, clientlen;
    ssize_t received;
    struct sockaddr_in echoserver; // structure address of server
    struct sockaddr_in echoclient; // structure address of client
    //create sock
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    //construct the server sockaddr_in structure
    memset(&echoserver, 0, sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_addr.s_addr = INADDR_ANY;
    echoserver.sin_port = htons(atoi(argv[1]));

    //bind socket to handle incoming messages at given port
    serverlen = sizeof(echoserver);
    if(bind(sock, (struct sockaddr*) &echoserver, serverlen) < 0)
    {
        perror("Failed to bind server socket");
        exit(EXIT_FAILURE);
    }

    if(listen(sock, 64) == -1)
    {
        perror("listen");
        exit(1);
    }

    //run until cancelled
    while((newSock = accept(sock, (struct sockaddr*) &echoclient, &clientlen)) != -1)
    {
        //forking
        cerr << "Client connected: " << inet_ntoa(echoclient.sin_addr) << '\n';
        if((received = read(newSock, buffer, 256)) == -1)
        {
            perror("read failed");
            exit(2);
        }

        if((write(newSock, buffer, received)) == -1)
        {
            perror("write mismatch");
            exit(3);
        }
        close(newSock);
    }
    close(sock);

    return 0;
}