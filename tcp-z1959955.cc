/* doc
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <cstring>
#include <iostream>
#include <string>
using namespace std;


void chomp(char *s);
int main(int argc, char *argv[])
{
    char buffer[257];
    int sock, newSock;
    socklen_t serverlen, clientlen;
    ssize_t received;
    struct sockaddr_in echoserver; // structure address of server
    struct sockaddr_in echoclient; // structure address of client
    DIR* dire_argpath;
    struct dirent *dire_argpath_dirent;

    //exit if not enough arguments
    if(argc < 2)
    {
        perror("Usage: ./z1959955 port path");
        exit(EXIT_FAILURE);
    }

    //open dir passed in
    dire_argpath = opendir(argv[2]);
    if(dire_argpath == 0) // exit if path doesn't exist
    {
        perror(argv[2]);
        exit(EXIT_FAILURE);
    }

    /*while((dire_argpath_dirent = readdir(dire_argpath)) != NULL) // see whats in directory
    {
        cout << "Detected: " << dire_argpath_dirent->d_name << "\n";
    }*/

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
    //clientlen = sizeof(echoclient);
    while((newSock = accept(sock, (struct sockaddr*) &echoclient, &clientlen)) != -1)
    {
        if(fork())
        {
            close(newSock);
        }
        else
        {
            //forking
            cerr << "Client connected: " << inet_ntoa(echoclient.sin_addr) << '\n';
            if((received = read(newSock, buffer, 256)) == -1) // read from client
            {
                perror("read failed");
                exit(2);
            }

            buffer[received] = '\0';
            chomp(buffer);

            cout << "Client wrote: " << buffer << "\n";
            //looking for file or directory
            string client_req = string(buffer);
            //trim off GET
            cout << "Before trim: " << client_req << "\n";
            size_t pos = client_req.find("GET ");
            client_req.erase(pos, 4);
            cout << "After trim: " << client_req << "\n";
            cout << "SIZE: " << client_req.size() << "\n";

            //no directory passed in, just use webroot: req = GET /
            if(client_req.compare("/") == 0)
            {
               //check directory for an index.html
               while((dire_argpath_dirent = readdir(dire_argpath)) != NULL) // look through dir
               {
                   string f_name = dire_argpath_dirent->d_name;
                   if(f_name.compare("index.html") == 0)
                   {
                       //found index.html in directory
                   }
                   else
                   {
                       if(f_name[0] != '.') // omit files starting with .
                       {
                           cout << f_name << " ";
                       }
                   }

               }
               cout << "\n";
            }
            else
            {
                // its not just /, there is more, now detect if searching for file or dir

                //is the last character in string a / <-- signifies dir
                string temp_cl_rq = client_req;

                size_t cl_req_size = temp_cl_rq.length();
                cout << "SIZE: " << cl_req_size << '\n';
                //size_t tp_rq_size = temp_cl_rq.length();
                char ending = temp_cl_rq.back();
                cout << "Ending: " << ending << "\n";
                //char endingC = client_req.at(cl_req_size-2);
                //size_t final_pos = temp_cl_rq.find_last_of("/");
                if(ending == '/')
                {
                    //searching for dir
                    //cout << "/ index: " << final_pos << '\n';
                    cout << "YOURE LOOKING FOR A DIR!" << '\n';
                }
                else
                {
                    cout << "Looking for a file" << '\n';
                }
                //cout << "endingC: " << endingC << '\n';

            }

            if((write(newSock, buffer, received)) == -1) // write to client
            {
                perror("write mismatch");
                exit(3);
            }
            close(newSock);
            exit(0);
        }

    }
    close(sock);

    return 0;
}

void chomp(char *s)
{
    for(char *p = s + strlen(s)-1; *p == '\r' || *p == '\n'; p--)
    {
        *p = '\0';
    }
}