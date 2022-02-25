// Client side C/C++ program to demonstrate Socket programming
#include "socket_functs.h"


int create_socket_client() {
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
	// NOTE: change this to required server address.
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
	// cout<<"Connected to "<<serv_addr.sin_addr.s_addr<<endl;
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

int send_mkdir_stream(int socket, string dir_path) {
	string unpadded_size = std::to_string(dir_path.length());
	string total_size; 
	// // next 8 bytes = dir stream size;
	// for (int i = 0; i<8; i++) {
	// 	if (i < unpadded_size.length())
	// 		total_size += unpadded_size[i];
	// 	else 
	// 		total_size += "!";
	// }
	// std::cout<<"[LOG] : Creating directory:  " << dir_path;
	// std::cout<<"padded size: " << total_size << std::endl;
	// dir_path = total_size + dir_path;
	dir_path = "!" + dir_path;
	printf("sending this to server: %s", dir_path.c_str());
	send(socket, dir_path.c_str(), dir_path.size(), 0);
	return 0;

}

int send_data(int new_socket, std::string file_name_arg) {
	if (std::filesystem::is_directory(file_name_arg))
		return -1;
	char buffer[1000000] = {0};

	std::ifstream file_to_send(file_name_arg, std::ios::in | std::ios::binary);
	

	// send the buffer to the client
	std::string contents((std::istreambuf_iterator<char>(file_to_send)), std::istreambuf_iterator<char>());

	std::cout<<"[LOG] : Sending...\n";

	std::string file_name_with_extension = file_name_arg;

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
	cout<<"[LOG] : Sent data\n\n";
	return 0;
}


// int main()
// {
// 	// std::vector<string> files_ls = {"vcs.db", "a.out", "client.cpp", "server.cpp"};
// 	// std::vector<string> files_ls = {"README.md", ".gitignore"};
// 	std::vector<string> files_ls = {"./received", "./received/received.yml", "vcs.db", ".gitignore"};
// 	int sock = create_socket_client();
// 	for(int i = 0; i < files_ls.size(); i++) {
// 		// std::this_thread::sleep_for(std::chrono::milliseconds(4000));
// 		// keep looping till you get response from server and if first file, then send without getting response 
// 		cout<<"sending file: "<<files_ls[i]<<endl;
// 		if (std::filesystem::is_directory(std::filesystem::path(files_ls[i]))) {
// 			send_mkdir_stream(sock, files_ls[i]);
// 		} else {
// 			send_data(sock, files_ls[i]);
// 		}
// 		// keep receiving until you get response from server
// 		char response_arr[1024];
// 		while(1){
// 			int valread = recv(sock, response_arr, sizeof(response_arr), 0);
// 			if (valread > 0 )
// 				break;
// 			for(int i=0; i<valread; i++)
// 				cout<<response_arr[i];
// 			// cout<<"bytes received: "<<valread<<endl;
// 			printf("response: %s\n", response_arr);
// 			// in case server closes socket, valread is 0
// 			if (valread == 0) {
// 				cout<<"\nServer closed connection";
// 				exit(EXIT_FAILURE);
// 			}
// 		}
// 		// close(sock);
// 	}
// 	return 0;
// }
