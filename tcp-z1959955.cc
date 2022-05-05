/*
** Herbert Morales
** zID: Z1959955
** CSCI-330-0001
** Assignment: 10 Simple HTTP SERVER
**
**
** This program is a simple TCP server, it will accept requests in the form of GET / and attempt to search
 * for the dir or file that the client types in. If a directory is passed in then the program will search for
 * an index.html file, if one is present then it will send the contents of that file to the client. Otherwise
 * a list of the files in the directory will be shown, if the directory does not exist then an error occurs.
 * If a filename is passed in from the client then the program will search for that file, if found, the program
 * will attempt to send the contents of the file to the client, otherwise an error is shown.
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <cstring>
#include <iostream>
#include <string>
#include <fcntl.h>
using namespace std;

/**
* Read contents of a file into a buffer
*
* This function will take in a file descriptor,
* a buffer to put the contents of a file into,
* and the size of the buffer. It uses the read()
* function and passes in all 3 parameters, if
* an error occurs while reading the file into
* the buffer, it will return a -1 and  be handled as an error.
* Otherwise it returns the numbers written from the function.
*
* @param fd File descriptor
* @param buffer Buffer to put file contents into
* @param b_size Size of the buffer
* @result nr Numbers written from function
*****************************************************************************/
int r_file_to_buffer(int fd, char buffer[], int b_size);

/**
* Extract the file directory from a string
*
* This function will take in a string and
* extract only the directory from the string
* and return only the directory.
*
* @param s String to process
* @result dir A directory
*****************************************************************************/
string get_req_dir(string s);
/**
* Extract the file name from a string
*
* This function will take in a string and
* extract only the file name from the string
* and return only the file name.
*
* @param s String to process
* @result filename A file name
*****************************************************************************/
string get_req_filn(string s);
/**
* Removes trailing newlines and carriages
*
* This function will take in a string and
* extract iterate through the string to remove
* trailing newlines and carriages from the string.
*
* @param s String to process
*****************************************************************************/
void chomp(char *s);
int
main(int argc, char *argv[])
{
    char buffer[257];
    int sock, newSock;
    socklen_t serverlen, clientlen;
    ssize_t received;
    struct sockaddr_in echoserver; // structure address of server
    struct sockaddr_in echoclient; // structure address of client
    DIR* dire_argpath;


    //exit if not enough arguments
    if(argc < 2)
    {
        perror("Usage: ./z1959955 port path");
        exit(EXIT_FAILURE);
    }

    string wb_root_path = string(argv[2]);

    //open dir passed in
    dire_argpath = opendir(argv[2]);
    if(dire_argpath == 0) // exit if path doesn't exist
    {
        perror(argv[2]);
        exit(EXIT_FAILURE);
    }

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
        if(fork())
        {
            close(newSock); // parent
        }
        else
        {
            //child process
            cerr << "Client connected: " << inet_ntoa(echoclient.sin_addr) << '\n';
            if((received = read(newSock, buffer, 256)) == -1) // read from client
            {
                perror("read failed");
                exit(2);
            }

            buffer[received] = '\0';
            chomp(buffer); // remove trailing newline or carriage returns

            //looking for file or directory
            string client_req = string(buffer);
            //trim off GET
            size_t pos = client_req.find("GET ");
            client_req.erase(pos, 4);

            //no directory passed in, just use webroot: req = GET /
            if(client_req.compare("/") == 0)
            {
               //check directory for an index.html
               string webrt_files; // will store all file names in directory to display to user
               bool index_in_rt = false; // was the index found in directory

               DIR* dire_wbrtpath;
               struct dirent *dire_wbrtpath_dirent;

                //open dir passed in
                dire_wbrtpath = opendir(argv[2]);
                if(dire_wbrtpath == 0) // exit if path doesn't exist
                {
                    perror(argv[2]);
                    exit(EXIT_FAILURE);
                }
                //dir exists so look through dir for index.html
               while((dire_wbrtpath_dirent = readdir(dire_wbrtpath)) != NULL) // look through dir
               {
                   string f_name = dire_wbrtpath_dirent->d_name;
                   if(f_name.compare("index.html") == 0)
                   {
                       //found index.html in directory
                       index_in_rt = true;
                       break;
                   }
                   else
                   {
                       if(f_name[0] != '.') // omit files starting with .
                       {
                           webrt_files += (f_name + " ");
                       }
                   }

               }
               if(index_in_rt)
               {
                   //index.html is found in webroot
                   //write contents of index to buffer
                   string wbr_index = wb_root_path + "/index.html";
                   int fd = open(wbr_index.c_str(), O_RDONLY);
                   ssize_t nr;
                   nr = r_file_to_buffer(fd, buffer, (sizeof(buffer)-1)); // put contents of file into buffer for client

                   if(nr == -1) // exit if an error reading the file occurs
                   {
                       perror("reading file to buffer");
                       return 3;
                   }
                   close(fd);
               }
               else
               {
                   //index.html isnt found in webroot
                   //add webrt file list to buffer
                   strncpy(buffer, webrt_files.c_str(), sizeof(buffer)); // add file list to buffer
                   buffer[sizeof(buffer)-1] = '\0';
               }

            }
            else
            {
                // its not just /, there is more, now detect if searching for file or dir

                //is the last character in string a / <-- signifies dir
                string temp_cl_rq = client_req; // temp string to hold the directory sent by user/ or file

                char ending = temp_cl_rq.back(); // get the last character in the request string
                if(ending == '/') // is the last character a /, if so, then we are looking for a directory
                {
                    //searching for dir

                    //concatenate webroot and the path passed in by user
                    string final_path = argv[2] + client_req; // complete path to search for directory

                    DIR* dire_fnpath;
                    struct dirent *dire_fnpath_dirent;

                    dire_fnpath = opendir(final_path.c_str()); // open directory passed in by user
                    if(dire_fnpath == 0) // exit if path doesn't exist
                    {
                        string dir_not_ex = "Error: Directory " + final_path + " does not exist";
                        perror(dir_not_ex.c_str());
                        exit(EXIT_FAILURE);
                    }
                    //dir exists so find index.html
                    string files_dir; // will store all file names found in the directory
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
                            // omit files starting with .
                            if(fil_name[0] != '.')
                            {
                                files_dir += (fil_name + " ");
                            }
                        }
                    }
                    if(found_index)
                    {
                        //write contents of index to buffer
                        final_path = final_path + "index.html";
                        int fd = open(final_path.c_str(), O_RDONLY);
                        ssize_t nr;
                        nr = r_file_to_buffer(fd, buffer, (sizeof(buffer)-1)); // put contents of file into buffer for client

                        if(nr == -1) // exit if an error occurs while reading file into buffer
                        {
                            perror("reading file to buffer");
                            return 3;
                        }
                        close(fd);
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
                    //looking for file in dir

                    //split dir and filename from request
                    string req_filename = get_req_filn(client_req); // use helper function to get filename
                    string req_dir = get_req_dir(client_req); // use helper function to get only dir
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
                        //string not_found = "We found the file";

                        //write contents of index to buffer
                        final_req_dir = final_req_dir + req_filename;
                        int fd = open(final_req_dir.c_str(), O_RDONLY);
                        ssize_t nr;
                        nr = r_file_to_buffer(fd, buffer, (sizeof(buffer)-1)); // put contents of file into buffer for client
                        if(nr == -1) // exit if an error occurs while reading file into buffer
                        {
                            perror("reading file to buffer");
                            return 3;
                        }
                        close(fd);
                    }
                    else
                    {
                        //we didn't find it so write an error to buffer
                        string not_found = "Error: " + req_filename + " not found";
                        strncpy(buffer, not_found.c_str(), sizeof(buffer)); // add file list to buffer
                        buffer[sizeof(buffer)-1] = '\0';
                    }
                }

            }

            if((write(newSock, buffer, strlen(buffer))) == -1) // write to client
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

int
r_file_to_buffer(int fd, char buffer[], int b_size)
{
    //read file into buffer
    ssize_t nr;
    nr = read(fd, buffer, b_size);
    if(nr == -1) // exit if an error occurs while reading file into buffer
    {
        return nr;
    }

    buffer[nr] = 0;
    return nr;
}

string
get_req_filn(string s) // get filename from request
{
    size_t loc = s.find_last_of('/'); // find last occurrence of /
    return s.substr(loc +1); // return everything following loc
}

string
get_req_dir(string s) // get dir from request
{
    size_t loc = s.find_last_of('/'); // find last occurrence of /
    return s.substr(0, loc); // return a substring starting from index 0 to loc
}

void
chomp(char *s)
{
    for(char *p = s + strlen(s)-1; *p == '\r' || *p == '\n'; p--)
    {
        *p = '\0';
    }
}
