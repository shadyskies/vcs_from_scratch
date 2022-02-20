// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>
#define CHUNK_SIZE 1024
#define PORT 8050
using namespace std;

int receive_basic(int sock, char *buf, string &file_name, string &file_content)
{
	int size_recv , total_size = 0;
	int valread = recv(sock, buf, sizeof(buf), 0);
	string stream_size;
	string final_bytes;
	// get the total bytes
	for(int i=0; i<8; i++) {
		final_bytes.push_back(buf[i]);
		cout<<buf[i];
		if (buf[i] != '#')
			stream_size.push_back(buf[i]);
	}
	total_size += valread;

	// get the file name
	string tmp_file_name;
	char file_name_ls[128];
	valread = recv(sock, file_name_ls, sizeof(file_name_ls), 0);
	for(int i=0; i<valread; i++) {
		final_bytes.push_back(file_name_ls[i]);
		if (file_name_ls[i]!='0')
			tmp_file_name.push_back(file_name_ls[i]);
	}
	file_name = tmp_file_name;

	string tmp_file_content;
	// loop till entire file content is received
	while(1){
		char chunk[CHUNK_SIZE] = {};
		if (total_size >= atoi(stream_size.c_str()))
			break;
		else {
			valread = recv(sock, chunk, sizeof(chunk), 0);
			
			for (int i=0; i<valread; i++)
				tmp_file_content.push_back(chunk[i]);
			total_size += valread;
		}
	}
	cout<<"[LOG] File Size received: "<<tmp_file_content.size()<<endl;
	
	file_content = tmp_file_content;
	return file_content.size();
}


int create_socket() {
	int sock = 0;
	struct sockaddr_in serv_addr;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}
	return sock;
}

int receive(int sock) {
	char buffer[1000000] = {};
	string file_name;
	string file_content;
	int valread = receive_basic(sock, buffer, file_name, file_content);
	
	// int valread = read(sock, buffer, sizeof(buffer));

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
	int sock = create_socket();
	receive(sock);
	return 0;
}
