// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fstream>
#include <iostream>
#define PORT 8050
using std::to_string;
using std::cout;


int send_all(int socket, std::string final_bytes) {
	// sending first 8 bytes (stream size)
	send(socket, (final_bytes.substr(0, 8)).c_str(), 8, 0);
	// sending next 128 bytes (file_name)
	send(socket, (final_bytes.substr(8, 136)).c_str(), 128, 0);
	// sending data
	send(socket, (final_bytes.substr(136, final_bytes.size())).c_str(), final_bytes.size(), 0);

}


int create_socket() {
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

		// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	
	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
								sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}

	return new_socket;
}


int send(int new_socket, std::string file_name_arg) {
	char buffer[1000000] = {0};

	std::ifstream file_to_send(file_name_arg, std::ios::in | std::ios::binary);
	

	// send the buffer to the client
	std::string contents((std::istreambuf_iterator<char>(file_to_send)), std::istreambuf_iterator<char>());

	std::cout<<"[LOG] : Sending...\n";

	std::string file_name_with_extension = "vcs.db";

	std::cout<<"file name length: "<<file_name_with_extension.length()<<std::endl;
	// first 8 bytes is transmissi size, next 128 is file name, rest contents size
	std::string final_size;

	char file_name[128];
	for(int i=0; i<128; i++) {
		if (i < file_name_with_extension.length()) {
			file_name[i] = file_name_with_extension[i];
		} else {
			file_name[i] = '0';
		}
	}

	std::string final_bytes = file_name + contents;
	for (int i = 0; i<8; i++) {
		if (i < std::to_string(final_bytes.size() + 8).length()) {
			final_size.push_back(std::to_string(final_bytes.size() + 8)[i]);
		} else {
			final_size.push_back('!');
		}
	}

	final_size = final_size + final_bytes;
	final_bytes = final_size;
	std::cout<<"[LOG] File size: "<<contents.size()<<std::endl;
	std::cout<<"[LOG] : Transmission Data Size "<<final_bytes.length()<<" Bytes.\n";
	send_all(new_socket, final_bytes);
	// send(new_socket , final_bytes.c_str() , final_bytes.length() , 0 );
	cout<<"[LOG] : Sent data"<<std::endl;
	return 0;
}



int main(int argc, char const *argv[])
{
	int new_socket = create_socket();
	send(new_socket, "vcs.db");
	
}
