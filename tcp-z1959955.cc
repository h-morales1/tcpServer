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


string get_req_dir(string s);
string get_req_filn(string s);
void chomp(char *s);
int main(int argc, char *argv[])
{
    char buffer[257];
    //char reply[257];
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
               string webrt_files;
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
                           //cout << f_name << " ";
                           webrt_files += (f_name + " ");
                       }
                   }

               }
               //add webrt file list to buffer
                strncpy(buffer, webrt_files.c_str(), sizeof(buffer)); // add file list to buffer
                buffer[sizeof(buffer)-1] = '\0';
            }
            else
            {
                // its not just /, there is more, now detect if searching for file or dir

                //is the last character in string a / <-- signifies dir
                string temp_cl_rq = client_req;

                size_t cl_req_size = temp_cl_rq.length();
                cout << "SIZE: " << cl_req_size << '\n';
                char ending = temp_cl_rq.back();
                cout << "Ending: " << ending << "\n";
                if(ending == '/')
                {
                    //searching for dir

                    //concatenate webroot and the path passed in by user
                    string final_path = argv[2] + client_req;
                    DIR* dire_fnpath;
                    struct dirent *dire_fnpath_dirent;
                    dire_fnpath = opendir(final_path.c_str());
                    if(dire_fnpath == 0) // exit if path doesn't exist
                    {
                        perror(argv[2]);
                        // write error to buffer for client
                        //stpcpy(buffer, "")
                        exit(EXIT_FAILURE);
                    }
                    //dir exists so find index.html
                    string files_dir;
                    bool found_index = false;
                    while((dire_fnpath_dirent = readdir(dire_fnpath)) != NULL) // look through dir
                    {
                        string fil_name = dire_fnpath_dirent->d_name; // name of current file
                        if(fil_name.compare("index.html") == 0)
                        {
                            //index.html is found in this dir
                            found_index = true;
                            break;
                        }
                        else
                        {
                            // list all files in dir
                            cout << fil_name << '\n';
                            if(fil_name[0] != '.')
                            {
                                files_dir += (fil_name + " ");
                            }
                        }
                    }
                    if(found_index)
                    {
                        //write contents of index to buffer
                    }
                    else
                    {
                        //we didn't find index.html so list files in dir
                        strncpy(buffer, files_dir.c_str(), sizeof(buffer)); // add file list to buffer
                        buffer[sizeof(buffer)-1] = '\0';
                    }

                }
                else
                {
                    //looking for file in dir  /fileOne.html
                    //split dir and filename from request
                    string req_filename = get_req_filn(client_req);
                    string req_dir = get_req_dir(client_req);
                    req_dir = req_dir + "/";
                    //concatenate users directory with webroot
                    string final_req_dir = argv[2] + req_dir;

                   //prep file search
                    DIR* dire_freqpath;
                    struct dirent *dire_freqpath_dirent;

                    dire_freqpath = opendir(final_req_dir.c_str());
                    if(dire_freqpath == 0) // exit if path doesn't exist
                    {
                        perror(argv[2]);
                        // write error to buffer for client
                        //stpcpy(buffer, "")
                        exit(EXIT_FAILURE);
                    }
                    //dir does exist so we  can search for file
                    bool req_f_found = false;
                    while((dire_freqpath_dirent = readdir(dire_freqpath)) != NULL) // look through dir
                    {
                        string fdir_filname = dire_freqpath_dirent->d_name;
                        if(fdir_filname.compare(req_filename) == 0)
                        {
                            //file we are searching for is found
                            req_f_found = true;
                            break;
                        }
                        else
                        {
                            //we didn't find the file
                            req_f_found = false;
                        }
                    }
                    if(req_f_found)
                    {
                        //we found it so write its contents to buffer
                        string not_found = "We found the file";
                        strncpy(buffer, not_found.c_str(), sizeof(buffer)); // add file list to buffer
                        buffer[sizeof(buffer)-1] = '\0';
                    }
                    else
                    {
                        //we didn't find it so write an error to buffer
                        string not_found = "Error: " + req_filename + " not found";
                        strncpy(buffer, not_found.c_str(), sizeof(buffer)); // add file list to buffer
                        buffer[sizeof(buffer)-1] = '\0';
                    }
                    cout << "FILE: " << req_filename << '\n';
                    cout << "DIR: " << req_dir << '\n';
                    cout << "Looking for a file" << '\n';
                }

            }

            if((write(newSock, buffer, strlen(buffer))) == -1) // write to client n was changed from received to buffer size
            {
                perror("write mismatch");
                exit(3);
            }
            sleep(3);
            close(newSock);
            exit(0);
        }

    }
    close(sock);

    return 0;
}

string get_req_filn(string s) // get filename from request
{
    // /fileOne.html

    size_t loc = s.find_last_of('/');
    return s.substr(loc +1);
}

string get_req_dir(string s) // get dir from request
{
    // /dub/dub/fileOne.html OR /fileOne.html
    size_t loc = s.find_last_of('/');
    return s.substr(0, loc);
}

void chomp(char *s)
{
    for(char *p = s + strlen(s)-1; *p == '\r' || *p == '\n'; p--)
    {
        *p = '\0';
    }
}