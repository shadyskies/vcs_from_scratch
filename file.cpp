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
using std::pair;

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

/* takes vector as input and returns that with lines in .txt based on revision_type */
void get_file_paths(std::vector<string> &_files, string revision_type){
    if (std::filesystem::exists("revisions/" + revision_type + ".txt")) {
        std::ifstream _type ("revisions/" + revision_type + ".txt");
        string line;
        while(std::getline(_type,line)){
            _files.push_back(line);
        }
    }   
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

// NOTE: only called by status function in order to retrieve file_path stored in db
static int get_db_files_callback(void *param, int argc, char **argv, char **azColName) {
    int i;
    std::vector<pair<string, int>> *s1 = (std::vector<pair<string, int>> *)param;
    // cout<<"argv[0]="<<argv[0]<<endl;
    // cout<<"argv[1]="<<argv[1]<<endl;
    string file_path = argv[0];
    int file_size = atoi(argv[1]);
    pair<string, int> tmp(file_path, file_size);
    s1->push_back(tmp);
    return 0;
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
        // files table
        string query = "CREATE TABLE files (id INTEGER PRIMARY KEY DEFAULT 0, file_path VARCHAR(2048), created_at DATETIME DEFAULT CURRENT_TIMESTAMP, file_size INTEGER);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Files table created successfully\n");
        }
    }
    
    // commits table
    connection = sqlite3_open("vcs.db", &db);
    if (!connection){
        string query = "CREATE TABLE commits (id INTEGER PRIMARY KEY DEFAULT 0, message VARCHAR(256), local_created_at DATETIME DEFAULT CURRENT_TIMESTAMP, remote_created_at DATETIME NULL, num_files_changed INTEGER);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Commit table created successfully\n");
        }
    }

    // file_revisions table ( commit as fk )
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "CREATE TABLE file_revisions(id INTEGER, file_path VARCHAR(1024),is_added BOOLEAN, is_REMOVED BOOLEAN, is_modified BOOLEAN, commit_id INT, FOREIGN KEY(commit_id) REFERENCES commits(id) ON DELETE CASCADE);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Revisions table created successfully\n");
        }
    }
    sqlite3_close(db);
    return 0;
}

// create .txt file based on revision_type(added, modified, removed) called by status
int file_revision(std::vector<string> file_paths, string revision_type) {
    if (file_paths.empty())
        return 0;
    string tmp_file = "./revisions/" + revision_type + ".txt";
    if (std::filesystem::exists(tmp_file))
        std::filesystem::remove(tmp_file);

    std::vector<string> files_ls;
    listdir(files_ls);
    std::vector<string> _files;
    get_file_paths(_files, revision_type);
    // cout<<"file_path: "<<file_path;
    // cout<<"revision files:"<<_files.size()<<endl;
    // cout<<"files in dir:"<<files_ls.size()<<endl;
    
    for (int j=0; j<file_paths.size(); j++){
        // check if file_path exists in project dir
        for (int i=0; i<files_ls.size(); i++){
            if(strcmp(files_ls[i].c_str(), (file_paths[j]).c_str()) == 0) {
                // check if file_path exists in <added / modified / removed>.txt file
                if (std::find(_files.begin(), _files.end(),file_paths[j]) != _files.end() != 0) {
                    break;
                }
                std::ofstream added;
                added.open("revisions/"+ revision_type +".txt", std::ios_base::app);
                added <<file_paths[j]<<endl;
                break;
            }
        } 

    }
    return 0;
}


// get all files in directory, check for changed files
void commit(std::string message) {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);
    std::vector<string> added_files;
    get_file_paths(added_files, "added");

    // insert in files table
    if (!connection) {

        for(int i = 0; i < added_files.size(); i++) {
            std::filesystem::path p{added_files[i]};
            string size;
            if (std::filesystem::is_directory(p)) {
                size = "0";
            }
            else {
                size = to_string(std::filesystem::file_size(p));
            }
            string query = "INSERT INTO files(file_path, file_size) VALUES ('" + added_files[i] + "', '" + size + "');";
            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                fprintf(stdout, "Inserted successfully\n");
            }
        }
    }
    else {
        cout<<"could not open database";
    }
    // insert in commits table
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "INSERT INTO commits(message, num_files_changed) VALUES('" + message + "', " + to_string(added_files.size()) + ");";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);
        if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Inserted successfully\n");
        }
    }

    // TODO:
    // insert in file_revision table
     
    sqlite3_close(db);
    // delete file
    std::filesystem::remove("revisions/added.txt");
}


// TODO: add change for change in file_size
// get files status
void status () {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    connection = sqlite3_open("vcs.db", &db);

    // status vectors
    std::vector<pair<string, int>>db_files; // stores file_path, file_size
    std::vector<string> removed;
    std::vector<string> modified;

    if (!connection) {
        string query = "SELECT file_path, file_size FROM files;";

        connection = sqlite3_exec(db, query.c_str(), get_db_files_callback, &db_files, &errmessage);   
        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
        
        // get listdir
            std::vector<string> files_ls;
            listdir(files_ls);
            // cout<<"files in dir: "<<files_ls.size();
            // cout<<"db_files: "<<db_files.size();
            for(int i=0; i<db_files.size(); i++) {
                // check if file present in dir + db
                auto idx = std::find(files_ls.begin(), files_ls.end(), db_files[i].first);
                if (idx != files_ls.end()) {
                    std::filesystem::path p {files_ls[idx - files_ls.begin()]};
                    int size;
                    if (std::filesystem::is_directory(p)) {
                        size = 0;
                    } else {
                        size = std::filesystem::file_size(p);
                    }
                    // cout<<"size of files_ls: "<<size<<" "<<"size of db_files: "<<db_files[i].second<<endl;
                    // if even the size is same, then the file is unchanged
                    if (db_files[i].second == size){
                        // cout<<"unchanged: "<<db_files[i].first<<endl;
                        files_ls.erase(std::remove(files_ls.begin(), files_ls.end(), db_files[i].first), files_ls.end());
                    }
                    // file is modified
                    else {
                        files_ls.erase(std::remove(files_ls.begin(), files_ls.end(), db_files[i].first), files_ls.end());
                        modified.push_back(db_files[i].first);
                    }
                }
                // else add to removed
                else  
                    removed.push_back(db_files[i].first);
            }

            cout<<"Removed: "<<removed.size()<<endl;
            cout<<"added: "<<files_ls.size()<<endl;
            cout<<"Modified: "<<modified.size()<<endl;

            // remaining files are newly added files
            file_revision(files_ls, "added");
            for(int i = 0; i < files_ls.size(); i++){
                cout<<"\n[ADDED] "<<files_ls[i];
            }
            file_revision(modified, "modified");
            for (int i = 0; i < modified.size(); i++){
                cout<<"\n[MODIFIED]"<<modified[i];     
            } 
            file_revision(removed, "removed");
            for (int i = 0; i < removed.size(); i++){
                cout<<"\n[REMOVED] "<<removed[i];
            }
        }
    } else {
        cout<<"could not open database";
    }
    sqlite3_close(db);
}


// similar to git add, create temp file to store new files
int add(string file_path) {
    create_revisions_directory("revisions");
    std::vector<string> files_ls;
    listdir(files_ls);
    std::vector<string> added_files;
    get_file_paths(added_files, "added");
    // cout<<"added files:"<<added_files.size()<<endl;
    
    // check if file_path entered exists in project dir
    for (int i=0; i<files_ls.size(); i++){
        if(strcmp(files_ls[i].c_str(), ("./" + file_path).c_str()) == 0) {
            if (std::find(added_files.begin(), added_files.end(), "./" + file_path) != added_files.end() != 0) {
                cout<<"File already added!"<<endl;
                return 1;
            }
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
        return 1;
    }

    if (check_args(argv[1], commands)) {
        if (strcmp(argv[1], "add") == 0)
            add(argv[2]);
        if (strcmp(argv[1], "commit") == 0)
            commit(argv[2]);
        if (strcmp(argv[1], "status") == 0)
            status();
    }
    else {
        cout<<"Unknown command: "<<argv[1]<<endl<<"Run ./a.out <add/commit/push>"<<endl;
    }
    // create_file_revision("some");
    return 0;
}
