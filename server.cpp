// Server side C/C++ program to demonstrate Socket programming
#include "socket_functs.h"
namespace fs = std::filesystem;

int create_socket_server() {
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
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	cout<<"\u001b[32mClient connected successfully!\u001b[0m"<<endl;
	// dont accept connections till one transfer is complete
	close(server_fd); 
	return new_socket;
}


// receive the entire file, return -1 for no socket connection
int receive_basic(int sock, char *buf, string &file_name, string &file_content)
{
	int size_recv , total_size = 0;
	int valread = recv(sock, buf, sizeof(buf), 0);
	// cout<<"received value: "<<valread<<endl;
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
	// cout<<"total bytes: "<<stream_size;
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
	// cout<<"size_received: "<<total_size + valread<<endl;
	// loop till entire file content is received
	while(1){
		char chunk[CHUNK_SIZE] = {};
		if (total_size >= atoi(stream_size.c_str()))
			break;
		else {
			valread = recv(sock, chunk, sizeof(chunk), 0);
			if (valread == 0)
				return -1;
			// cout<<"bytes received: "<<valread<<endl;
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

	// cout<<"size of stream:";
	// for(int i = 0; i < 8; i++)
	// 	cout<<buffer[i];
	// cout<<"\n";

	file_name = fs::path(file_name).filename();

	std::cout<<"[LOG] : Data received "<<valread<<" bytes\n";
	std::cout<<"[LOG] : File Name: "<<file_name<<endl;
	std::cout<<"[LOG] : Saving data to file.\n";
	auto myfile = std::fstream("received/" + file_name, std::ios::out | std::ios::binary);
    myfile.write(file_content.c_str(), valread);
    myfile.close();
	std::cout<<"[LOG] : File Saved.\n\n";
	return 0;
}




int main(int argc, char const *argv[])
{
	int new_socket = create_socket_server();
	int flag = 0;
	while(1){
		// if connection inactive, create new socket, reset flag
		if (flag == 1) {
			flag = 0;
			close(new_socket);
			// cout<<"close socket return val:: "<<close(new_socket);
			new_socket = create_socket_server();
			// cout<<"new_socket: "<<new_socket<<endl;
		}
		int receive_val = receive_data(new_socket);
		// case of receiving data from client
		if (receive_val!=-1){
			string tmp = "temp string";
			// send data to ack data transfer
			send(new_socket, tmp.c_str(), strlen(tmp.c_str())+1, 1);
		} else  {
			flag ++;
		}
		// close the socket so that client can send new data 
		// close(new_socket);
	}
	
}
