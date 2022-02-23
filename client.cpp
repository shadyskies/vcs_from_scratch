// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#define CHUNK_SIZE 1024
#define PORT 8050
using namespace std;


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
	int de = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (de < 0)
	{
		printf("\nConnection Failed \n");
		exit(EXIT_FAILURE);
		return -1;
	}
	cout<<"Connected to "<<serv_addr.sin_addr.s_addr<<endl;
	return sock;
}

// sends the entire data in chunks
int send_all(int socket, std::string final_bytes) {
	// sending first 8 bytes (stream size)
	send(socket, (final_bytes.substr(0, 8)).c_str(), 8, 0);
	// sending next 128 bytes (file_name)
	send(socket, (final_bytes.substr(8, 136)).c_str(), 128, 0);
	// sending data
	send(socket, (final_bytes.substr(136, final_bytes.size())).c_str(), final_bytes.size(), 0);
	// send(socket, final_bytes.c_str(), final_bytes.size(), 0);
	return 0;
}

// receive the entire file
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




int send_data(int new_socket, std::string file_name_arg) {
	char buffer[1000000] = {0};

	std::ifstream file_to_send(file_name_arg, std::ios::in | std::ios::binary);
	

	// send the buffer to the client
	std::string contents((std::istreambuf_iterator<char>(file_to_send)), std::istreambuf_iterator<char>());

	std::cout<<"[LOG] : Sending...\n";

	std::string file_name_with_extension = file_name_arg;

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

int receive_data(int sock) {
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


int main()
{
	std::vector<string> files_ls = {"vcs.db", "a.out", "client.cpp", "server.cpp"};
	// std::vector<string> files_ls = {"README.md", ".gitignore"};
	int sock = create_socket();
	for(int i = 0; i < files_ls.size(); i++) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		// keep looping till you get response from server and if first file, then send without getting response 
		cout<<"sending file: "<<files_ls[i]<<endl;
		send_data(sock, files_ls[i]);
		// keep receiving until you get response from server
		char response_arr[1024];
		while(1){
			int valread = recv(sock, response_arr, sizeof(response_arr), 0);
			if (valread > 0 )
				break;
			for(int i=0; i<valread; i++)
				cout<<response_arr[i];
			cout<<"bytes received: "<<valread<<endl;
			printf("response: %s\n", response_arr);
			// in case server closes socket, valread is 0
			if (valread == 0) {
				cout<<"\nServer closed connection";
				exit(EXIT_FAILURE);
			}
		}
		// close(sock);
		cout<<"creating a new socket..."<<endl;
	}
	return 0;
}
