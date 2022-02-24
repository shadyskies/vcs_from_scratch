#ifndef SOCKET_FUNCS
#define SOCKET_FUNCS

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <filesystem>

#define PORT 8050
#define CHUNK_SIZE 1024
using std::string;
using std::endl;
using std::cout;
using std::vector;


int create_socket_client();
int create_socket_server();
int send_all(int socket, string final_bytes);
int send_data(int socket, string file_name_arg);
int receive_basic(int sock, char *buf, string &file_name, string &file_content);
int receive_data(int sock);
int send_mkdir_stream(int socket, string dir_path);

#endif