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
int create_revisions_directory(string text) {
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

void insert(string file_path);

// get current time as string
string get_curr_time() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);      
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");
    string timestamp = oss.str();
    return timestamp;   
}


static int callback(void *param, int argc, char **argv, char **azColName) {
    // cout<<"inside callback"<<endl;
    // cout<<"param[0]="<<((string*)param)[0]<<endl;
    // cout<<"param[1]="<<((string*)param)[1]<<endl;
    std::string &s = *static_cast<std::string*>(param);
    int i;
    // for(i = 0; i<argc; i++) {
    //     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    // }
    // if count == 0, insert
    if ((!strcmp(azColName[0], "COUNT(*)")) && (atoi(argv[0]) == 0)) {
        // set flag as 1
        string *flag = &((string*)param)[0];
        *flag = "1";
        return 1; 
        
    }
    printf("\n");
    return 0;
}


void insert(string file_path, sqlite3 *db) {
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);   

    if (!connection) {
        string timestamp = get_curr_time();
        string query = "INSERT INTO files(file_path, created_at) VALUES ('" + file_path + "', '" + timestamp +"');";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Records created successfully\n");
        }
    }
}

// create files table in sqlite
int create_table() {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);

    if (!connection) {
        string query = "CREATE TABLE files (id INTEGER PRIMARY KEY DEFAULT 0, file_path VARCHAR(2048), created_at DATETIME, file_size INTEGER);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Table created successfully\n");
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
                string query = "SELECT COUNT(*) FROM files WHERE file_path='" + file.path().u8string() + "';";
                string file_path = file.path().u8string();
                std::pair<string, string> pair1;
                pair1 = make_pair(file_path, file.file_size());
                
                connection = sqlite3_exec(db, query.c_str(), callback, &pair1, &errmessage);   
                if (atoi(pair1.first.c_str()) == 1) {
                    insert(file_path, db);
                }
            }
        }
    }
    else {
        cout<<"could not open database";
    }
    sqlite3_close(db);
    
}

/* get all files in the directory in vector */
void listdir(std::vector<string> &files_ls) {
    std::string path = ".";
    for (const auto & file : recursive_directory_iterator(path)) {
        if (file.path().u8string().string::find(".git") == string::npos) {
            files_ls.push_back(file.path().u8string());
        }
    }
}


// NOTE: only called by status function in order to retrieve file_path stored in db
static int get_db_files_callback(void *param, int argc, char **argv, char **azColName) {
    int i;
    std::vector<string> *s1 = (std::vector<string> *)param;
    s1->push_back(argv[0]);
    return 0;
}

// get files status
void status () {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);

    // status vectors
    std::vector<string> db_files;
    std::vector<string> removed;

    if (!connection) {
        string query = "SELECT (file_path) FROM files;";

        connection = sqlite3_exec(db, query.c_str(), get_db_files_callback, &db_files, &errmessage);   
        // get listdir
        std::vector<string> files_ls;
        listdir(files_ls);
        // cout<<"files in dir: "<<files_ls.size();

        for(int i=0; i<db_files.size(); i++) {
            // if present in both, remove from files_ls
            if (std::find(files_ls.begin(), files_ls.end(), db_files[i]) != files_ls.end()) 
                files_ls.erase(std::remove(files_ls.begin(), files_ls.end(), db_files[i]), files_ls.end());
            // else add to removed
            else  
                removed.push_back(db_files[i]);
        }
        // remaining files are newly added files
        for (int i = 0; i < removed.size(); i++)
            cout<<"\n[REMOVED] "<<removed[i];
        for(int i = 0; i < files_ls.size(); i++)
            cout<<"\n[ADDED] "<<files_ls[i];     

    } else {
        cout<<"could not open database";
    }
    sqlite3_close(db);
}


// TODO: add verification for adding files only once
// similar to git add, create temp file to store new files
int add(string file_path) {
    create_revisions_directory("revisions");
    std::vector<string> files_ls;
    listdir(files_ls);
    for (int i=0; i<files_ls.size(); i++){
        if(strcmp(files_ls[i].c_str(), ("./" + file_path).c_str()) == 0) {
            std::ofstream added;
            added.open("revisions/added.txt", std::ios_base::app);
            added << "./" + file_path<<endl;
            cout<<"\nAdded file!";
            return 0;
        }
    } 
    cout <<"\nFile not found!";
    return 1;
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
    std::vector<std::string> commands {"status", "add", "commit", "push", "remove"};
    // check if empty repository
    create_table();
    if (argc == 1) {
        cout<<"Incorrect usage. Run ./a.out <add/commit/push>"<<endl;
    }

    if (check_args(argv[1], commands)) {
        if (strcmp(argv[1], "add") == 0)
            add(argv[2]);
        if (strcmp(argv[1], "commit") == 0)
            commit();
        if (strcmp(argv[1], "status") == 0)
            status();
    }
    // create_file_revision("some");
    return 0;
}
