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
#define CHUNK_SIZE 1024
using std::to_string;
using std::cout;
using std::string;
using std::endl;

// sends the entire data in chunks
int send_all(int socket, std::string final_bytes) {
	// sending first 8 bytes (stream size)
	send(socket, (final_bytes.substr(0, 8)).c_str(), 8, 0);
	// sending next 128 bytes (file_name)
	send(socket, (final_bytes.substr(8, 136)).c_str(), 128, 0);
	// sending data
	send(socket, (final_bytes.substr(136, final_bytes.size())).c_str(), final_bytes.size(), 0);
	return 0;
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
	cout<<"-----------------------------------";

	cout<<"\nListening for connections"<<endl;
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	cout<<"No connection accepted as of now....";
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	cout<<"\u001b[32mClient connected successfully!\u001b[0m"<<endl;

	return new_socket;
}


int send_data(int new_socket, std::string file_name_arg) {
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

// receive the entire file, return -1 for no socket connection
int receive_basic(int sock, char *buf, string &file_name, string &file_content)
{
	int size_recv , total_size = 0;
	int valread = recv(sock, buf, sizeof(buf), 0);
	if (valread == 0)
		return -1;
	string stream_size;
	string final_bytes;
	// get the total bytes
	for(int i=0; i<8; i++) {
		final_bytes.push_back(buf[i]);
		// cout<<buf[i];
		if (buf[i] != '!')
			stream_size.push_back(buf[i]);
	}
	cout<<"total bytes: "<<stream_size;
	total_size += valread;

	// get the file name
	string tmp_file_name;
	char file_name_ls[128];
	valread = recv(sock, file_name_ls, sizeof(file_name_ls), 0);
	// return if 0 bytes received (no connection)
	for(int i=0; i<valread; i++) {
		final_bytes.push_back(file_name_ls[i]);
		if (file_name_ls[i]!='0')
			tmp_file_name.push_back(file_name_ls[i]);
	}
	file_name = tmp_file_name;

	string tmp_file_content;
	cout<<"size_received: "<<total_size + valread<<endl;
	// loop till entire file content is received
	while(1){
		char chunk[CHUNK_SIZE] = {};
		if (total_size >= atoi(stream_size.c_str()))
			break;
		else {
			valread = recv(sock, chunk, sizeof(chunk), 0);
			if (valread == 0)
				return -1;
			cout<<"bytes received: "<<valread<<endl;
			for (int i=0; i<valread; i++)
				tmp_file_content.push_back(chunk[i]);
			total_size += valread;
		}
	}
	// bytes are looping to start
	tmp_file_content = tmp_file_content.substr(0, atoi(stream_size.c_str()) - 136);
	cout<<"[LOG] File Size received: "<<tmp_file_content.size()<<endl;
	
	file_content = tmp_file_content;
	return file_content.size();
}




// return 0 for all data received and -1 for no socket connection
int receive_data(int sock) {
	char buffer[1000000] = {};
	string file_name;
	string file_content;
	int valread = receive_basic(sock, buffer, file_name, file_content);
	// int valread = read(sock, buffer, sizeof(buffer));
	if (valread == -1)
		return -1;

	cout<<"size of stream:";
	for(int i = 0; i < 8; i++)
		cout<<buffer[i];
	cout<<"\n";


	cout<<"\nFile name: "<<file_name<<endl;
	std::cout<<"[LOG] : Data received "<<valread<<" bytes\n";
	std::cout<<"[LOG] : File Name: "<<file_name<<endl;
	std::cout<<"[LOG] : Saving data to file.\n";
	auto myfile = std::fstream("received/" + file_name, std::ios::out | std::ios::binary);
    myfile.write(file_content.c_str(), valread);
    myfile.close();
	std::cout<<"[LOG] : File Saved.\n";
	return 0;
}




int main(int argc, char const *argv[])
{
	int new_socket = create_socket();
	int flag = 0;
	while(1){
		// if connection inactive, create new socket, reset flag
		if (flag == 1000) {
			flag = 0;
			cout<<"close socket return val:: "<<close(new_socket);
			new_socket = create_socket();
		}
		int receive_val = receive_data(new_socket);
		// case of receiving data from client
		if (receive_val!=-1){
			cout<<"sending response to client as file is received!"<<endl;
			string tmp = "temp string";
			cout<<"sending receive message..."<<send(new_socket, tmp.c_str(), strlen(tmp.c_str())+1, 1);
		} else  {
			flag ++;
		}
		// close the socket so that client can send new data 
		// close(new_socket);
	}
	
}
