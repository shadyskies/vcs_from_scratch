#include <iostream>
#include <bits/stdc++.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <iostream>
#include <sqlite3.h>
#include <stdlib.h>

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

struct stat info;

void write_file(string text) {
    std::ofstream outfile;
    outfile.open("Gfg.txt", std::ios_base::app); // append instead of overwrite
    outfile << text; 
}

// create directory to store file revisions
int create_directory(string text) {
    const char* path = text.c_str();

    if( stat( path, &info ) != 0 ) {
        if (mkdir(path, 0777) == -1)
        cerr << "Error :  " << strerror(errno) << endl;
    }
    else if( info.st_mode & S_IFDIR )  
        return 2;    
    else
        cout<<"that is not a directory";
    return 0;
}

static int callback(void *param, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   // if count = 0, insert data;
   cout<<"params = "<<param<<endl;
   printf("\n");
   return 0;
}

// create files table in sqlite
int create_table() {
    cout <<"connecting to database";
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);

    if (!connection) {
        string query = "CREATE TABLE files (id INTEGER PRIMARY KEY DEFAULT 0, file_path VARCHAR(2048), created_at DATETIME);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Records created successfully\n");
        }
    }
    sqlite3_close(db);
    return 0;
}



// get all files in directory, check for changed files
void commit() {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);

    if (!connection) {
        string path = ".";
        for (const auto & file : recursive_directory_iterator(path)) {
            if (file.path().u8string().string::find(".git") == string::npos){
                // check if exists in db
                cout<<"file: "<<file.path().u8string()<<endl;
                string query = "SELECT COUNT(*) FROM files WHERE file_path='" + file.path().u8string() + "';";
                string file_path = "./file";
                connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

                // string query = "INSERT INTO files(file_path, created_at) VALUES('";
                // cout << file.path().u8string() << endl;
                // auto t = std::time(nullptr);
                // auto tm = *std::localtime(&t);      
                // std::ostringstream oss;
                // oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
                // string timestamp = oss.str();
                // query += file.path().u8string() + "','" + timestamp + "');";
                // // cout<<"query: "<<query<<endl;
                // connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

                // if(connection != SQLITE_OK ){
                //     fprintf(stderr, "SQL error: %s\n", errmessage);
                //     sqlite3_free(errmessage);
                // } else {
                //     fprintf(stdout, "Records created successfully\n");
                // }
            }
        }
    }
    else {
        cout<<"could not open database";
    }
    sqlite3_close(db);
    
}


/* NOTE: deprecated for now */
// int create_file_revision(string text) {
// 	fstream file;
//     auto t = std::time(nullptr);
//     auto tm = *std::localtime(&t);
//     std::hash<string> hash_string;


//     std::ostringstream oss;
//     oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
//     string timestamp = oss.str();
    
//     string filepath = "revisions/" + timestamp + ".txt";
//     file.open(filepath,std::ios_base::app);
//     file <<hash_string(timestamp)<< endl << endl;
//     file << "This file was auto-generated on "<<timestamp;
//     if(!file)
//     {
//         cout<<"Error in creating file!!!";
//         return 1;
//     }
//     string path = ".";
//     std::ofstream file_list;
//     file_list.open(filepath,ios::in | ios::out);
    
//     for (const auto & file : recursive_directory_iterator(path)) {
//         int count = 0;
//         if (file.path().u8string().string::find(".git") == string::npos){
//             // open file_list and add new files
//             std::ifstream file_list;
//             file_list.open("revisions/list.txt", std::ios_base::app);
//             if (file_list.is_open()) {
//                 string line;
//                 while(std::getline(file_list, line)){
//                     if (line.string::find(file.path().u8string()) != string::npos){
//                         count++;
//                     }
//                 }
//             }
//             if (count==0) {
//                 cout << file.path().u8string() << endl;
//                 std::ofstream tmp;
//                 tmp.open("revisions/list.txt", std::ios_base::app);
//                 tmp <<file.path().u8string() << endl;
//                 tmp.close();
//             }
//             file_list.close();
//         }
//     }
//     cout<<"File created successfully.";
//     file.close();
//     return 0;
// }

bool check_args(const std::string &value, const std::vector<std::string> &array)
{
    return std::find(array.begin(), array.end(), value) != array.end();
}


// Driver code
int main(int argc, char* argv[])
{
    std::vector<std::string> commands {"add", "commit", "push", "remove"};
    for(int i = 0; i < argc; ++i)
        cout<<argv[i]<<endl;
    // check if empty repository
    if(create_directory("revisions") != 2)
        create_table();
    if (argc == 1) {
        cout<<"Incorrect usage. Run ./a.out <add/commit/push>"<<endl;
    }

    if (check_args(argv[1], commands)) {
        cout<<"executing commands";
        commit();
    }
    // create_file_revision("some");
    return 0;
}
