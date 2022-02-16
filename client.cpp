// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>
#define PORT 8080
using namespace std;

int main(int argc, char const *argv[])
{
	int sock = 0;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
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
	// send(sock , hello , strlen(hello) , 0 );
	// printf("Hello message sent\n");
	char buffer[1000000] = {};
	int valread = read(sock , buffer, 1000000);
	std::string file_name;
	for(int i=0; i<128; i++) {
		cout<<buffer[i];
		if (buffer[i]!='0') {
			cout<<buffer[i];
			file_name.push_back(buffer[i]);
		}
	}
	cout<<"\nFile name: "<<file_name<<endl;
	std::cout<<"[LOG] : Data received "<<valread<<" bytes\n";
	std::cout<<"[LOG] : Saving data to file.\n";
	auto myfile = std::fstream(file_name + "received", std::ios::out | std::ios::binary);
    myfile.write((char*)&buffer[128], valread);
    myfile.close();
	std::cout<<"[LOG] : File Saved.\n";
	return 0;
}
