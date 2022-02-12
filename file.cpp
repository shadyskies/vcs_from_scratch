#include <iostream>
#include <bits/stdc++.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <filesystem>

using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;
using std::fstream;
using std::ios;
using std::filesystem::recursive_directory_iterator;
using std::to_string;

void write_file(string text) {
    std::ofstream outfile;
    outfile.open("Gfg.txt", std::ios_base::app); // append instead of overwrite
    outfile << text; 
}

// create directory to store file revisions
void create_directory(string text) {
    const char* path = text.c_str();
    if (mkdir(path, 0777) == -1)
        cerr << "Error :  " << strerror(errno) << endl;
}

// get all files in directory
void get_files() {
    string path = ".";

    for (const auto & file : recursive_directory_iterator(path)) {
        if (file.path().u8string().string::find(".git") == string::npos){
        cout << file.path() << endl;
        }
    }
}

int create_file_revision(string text) {
	fstream file;
    file.open("tmp.txt",ios::out);
    file << text;
    if(!file)
    {
        cout<<"Error in creating file!!!";
        return 1;
    }

    cout<<"File created successfully.";
    file.close();
    return 0;
}


// Driver code
int main()
{
    write_file("this is the text appended");
    write_file("\nthis is some new text!");
    get_files();
    create_directory("revisions");
    create_file_revision("some");
    return 0;
}
