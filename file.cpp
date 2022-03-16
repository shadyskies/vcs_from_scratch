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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "socket_functs.h"

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
using std::filesystem::directory_iterator;
namespace fs = std::filesystem;

void status();

struct stat info;
std::vector<string> IGNORE {".git", "revisions"};

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
    if (std::filesystem::exists("revisions/files/" + revision_type + ".txt")) {
        std::ifstream _type ("revisions/files/" + revision_type + ".txt");
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

static int get_latest_commit_id(void *param, int argc, char **argv, char **azColName ) {
    int *commit_id = (int*)param;
    *commit_id = atoi(argv[0]);
    return 0;
}

static int get_files_in_commit(void *param, int argc, char **argv, char **azColName) {
    std::vector<string> *s1 = (std::vector<string> *)param;
    s1->push_back(argv[0]);
    return 0;
}


// static int get_commits_to_push(void *param, int argc, char **argv, char **azColName) {
//     std::vector<string> *s1 = (std::vector<string> *)param;

// }


/* get all files in the directory in vector */
void listdir(std::vector<string> &files_ls) {
    std::string path = ".";
    for (const auto & file : recursive_directory_iterator(path)) {
        if (file.path().u8string().string::find(".git") == string::npos && file.path().u8string().string::find("revisions") == string::npos) {
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
        string query = "CREATE TABLE files (id INTEGER PRIMARY KEY DEFAULT 0, file_path VARCHAR(2048), created_at DATETIME DEFAULT(datetime('now', 'localtime')), file_size INTEGER);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            // fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Files table created successfully\n");
        }
    }
    
    // commits table
    connection = sqlite3_open("vcs.db", &db);
    if (!connection){
        string query = "CREATE TABLE commits (id INTEGER PRIMARY KEY DEFAULT 0, message VARCHAR(256), local_created_at DATETIME DEFAULT(datetime('now', 'localtime')), remote_created_at DATETIME NULL, num_files_changed INTEGER);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            // fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Commit table created successfully\n");
        }
    }

    // file_revisions table ( commit as fk )
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "CREATE TABLE file_revisions(id INTEGER PRIMARY KEY DEFAULT 0, file_path VARCHAR(1024),is_added BOOLEAN, is_removed BOOLEAN, is_modified BOOLEAN, commit_id INT, FOREIGN KEY(commit_id) REFERENCES commits(id) ON DELETE CASCADE);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            // fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Revisions table created successfully\n");
        }
    }
    // commits_to_push
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "CREATE TABLE commits_to_push (id INTEGER PRIMARY KEY DEFAULT 0, commit_id INT, FOREIGN KEY(commit_id) REFERENCES commits(id) ON DELETE CASCADE);";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            // fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            fprintf(stdout, "Revisions table created successfully\n");
        }   
    }

    sqlite3_close(db);
    return 0;
}

// create .txt file based on revision_type(added, modified, removed) called only by status method
// file_paths --> file paths to store on
int file_revision(std::vector<string> file_paths, string revision_type) {
    if (file_paths.empty())
        return 0;
    cout<<"creating dir..."<<endl;
    create_revisions_directory("revisions/files/");
    
    string tmp_file = "./revisions/files/" + revision_type + ".txt";
    
    std::vector<string> _files_in_file;
    get_file_paths(_files_in_file, revision_type);
    
    // if revision_type is removed, create file with revisions
    for (int i=0; i<file_paths.size(); i++) {
        // if path already added to file, skip
        if (std::find(_files_in_file.begin(), _files_in_file.end(), file_paths[i]) == _files_in_file.end() == 0) {
            continue;
        }
        else {
            std::ofstream added;
            added.open("revisions/files/" + revision_type +".txt", std::ios_base::app);
            added<<file_paths[i]<<endl;
            added.close();
        }
    }
    return 0;
}


// get all files in directory, check for changed files
void commit(std::string message) {
    create_revisions_directory("revisions/commits/");
    // in case there are changes and added.txt, modified files are removed
    status();

    // if revisions/files folder empty / nothing to commit
    if (!fs::exists(fs::path("revisions/files/"))) {
        cout<<"Nothing to commit!\n";
        return;
    }

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
                // fprintf(stdout, "Inserted successfully\n");
            }
        }
    }
    else {
        cout<<"could not open database";
    }
        
    // modified files
    connection = sqlite3_open("vcs.db", &db);
    std::vector<string> modified_files;
    get_file_paths(modified_files, "modified");
    if (!connection) {

        for(int i = 0; i < modified_files.size(); i++) {
            std::filesystem::path p{modified_files[i]};
            string size;
            if (std::filesystem::is_directory(p)) {
                size = "0";
            }
            else {
                size = to_string(std::filesystem::file_size(p));
            }
            string query = "UPDATE files SET file_size = " + size + " WHERE file_path='" + modified_files[i] + "';";
            // cout<<"query: "<<query <<endl;
            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                // fprintf(stdout, "Updated successfully\n");
            }
        }
    }
    else {
        cout<<"could not open database";
    }

    // removed files
    std::vector<std::string> removed_files;
    get_file_paths(removed_files, "removed");
    if (!connection) {

        for(int i = 0; i < removed_files.size(); i++) {
            string query = "DELETE FROM files WHERE file_path='" + removed_files[i] + "';";
            // cout<<"query: "<<query <<endl;

            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                // fprintf(stdout, "Removed successfully\n");
            }
        }
    }
    else {
        cout<<"could not open database";
    }


    // insert in commits table
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "INSERT INTO commits(message, num_files_changed) VALUES('" + message + "', " + to_string(modified_files.size()) + ");";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);
        if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
        } else {
            // fprintf(stdout, "Inserted successfully\n");
        }
    }

    // get the commit id for latest commit
    connection = sqlite3_open("vcs.db", &db);
    int commit_id = 0;
    if (!connection) {
        string query = "SELECT id from commits ORDER BY local_created_at DESC LIMIT 1;";
        connection = sqlite3_exec(db, query.c_str(), get_latest_commit_id, &commit_id, &errmessage);     
        cout<<"Latest commit id: "<<commit_id<<endl;
        if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
        } else {
            // fprintf(stdout, "Inserted successfully\n");
        }
    }

    // file_revisions table
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        // file revisions for added files;
        for(int i = 0; i < added_files.size(); i++) {
            string query = "INSERT INTO file_revisions(file_path, is_added, commit_id) VALUES ('" + added_files[i] + "',true, " + to_string(commit_id) + ")";
            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                // fprintf(stdout, "Inserted successfully\n");
            }
        }
    }

    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        // file revisions for modified files;
        for(int i = 0; i < modified_files.size(); i++) {
            string query = "INSERT INTO file_revisions(file_path, is_modified, commit_id) VALUES ('" + modified_files[i] + "',true, " + to_string(commit_id) + ")";
            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                // fprintf(stdout, "Inserted successfully\n");
            }
        }
    }

    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        // file revisions for removed files;
        for(int i = 0; i < removed_files.size(); i++) {
            string query = "INSERT INTO file_revisions(file_path, is_removed, commit_id) VALUES ('" + removed_files[i] + "',true, " + to_string(commit_id) + ")";
            connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

            if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
            } else {
                // fprintf(stdout, "Inserted successfully\n");
            }
        }
    }
     
    // commits_to_push
    connection = sqlite3_open("vcs.db", &db);
    if (!connection) {
        string query = "INSERT INTO commits_to_push(commit_id) VALUES (" + to_string(commit_id) + ");";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);   

        if( connection != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            // fprintf(stdout, "Inserted successfully\n");
        }
    }

    sqlite3_close(db);
    // delete file
    std::filesystem::remove("revisions/added.txt");

    // create specific commit folder and copy all files
    string commit_path = "revisions/commits/" + to_string(commit_id) + "/";
    create_revisions_directory(commit_path);
    std::vector<string> files_ls;
    listdir(files_ls);
    
    // TODO: create algo for specific files instead of whole directory
    fs::copy_options copyOptions = fs::copy_options::update_existing | fs::copy_options::recursive;
    for(int i = 0; i < files_ls.size(); i++) {
        try{
            auto file_path = std::filesystem::path(commit_path + files_ls[i]);
            std::filesystem::copy(files_ls[i], file_path, copyOptions);
        }
        catch(int i) {
            cout<<"Exception: "<<i<<endl;
        }
    }

    // remove revisions/files/ dir
    fs::remove_all(fs::path("revisions/files"));
}


// get files status
void status () {
    // rm prev dir to update values
    fs::remove_all(fs::path("revisions/files/"));
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
            cout<<"Added: "<<files_ls.size()<<endl;
            cout<<"Modified: "<<modified.size()<<endl;

            // remaining files are newly added files
            file_revision(files_ls, "added");
            for(int i = 0; i < files_ls.size(); i++){
                cout<<"\n\u001b[32m[ADDED] "<<files_ls[i]<<"\u001b[0m";
            }
            file_revision(modified, "modified");
            for (int i = 0; i < modified.size(); i++){
                cout<<"\n\u001b[33m[MODIFIED]"<<modified[i]<<"\u001b[0m";     
            } 
            file_revision(removed, "removed");
            for (int i = 0; i < removed.size(); i++){
                cout<<"\n\u001b[31m[REMOVED]"<<removed[i]<<"\u001b[0m";
            }
        }
    } else {
        cout<<"could not open database";
    }
    cout<<endl;
    sqlite3_close(db);
}

void show_log() {
    sqlite3 *db;
    char *errmessage = 0;
    int connection;
    // number of commits to see log of
    int n = 1;
    cout<<"Enter number of commits to see log of: ";
    cin>>n;
    connection = sqlite3_open("vcs.db", &db);
    int commit_id = 0;
    if (!connection) {
        string query = "SELECT id from commits ORDER BY local_created_at DESC LIMIT 1;";
        connection = sqlite3_exec(db, query.c_str(), get_latest_commit_id, &commit_id, &errmessage);     
        if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
        } else {}
    }
    while (n--) {
        if(commit_id == 0)
            break;
        connection = sqlite3_open("vcs.db", &db);
        std::vector<string> file_revisions;
        string query = "SELECT (file_path) from file_revisions WHERE commit_id=" + to_string(commit_id) + ";";
        connection = sqlite3_exec(db, query.c_str(), get_files_in_commit, &file_revisions, &errmessage);
        if (connection != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            // print the files
            cout<<"Commit ID: "<<commit_id<<endl;
            for (int i = 0; i < file_revisions.size(); i++) 
                cout<<file_revisions[i]<<endl;
        }
        cout<<endl;
        commit_id--;
    }

}


int push_to_server() {
    // get the files
    sqlite3 *db;
    char *errmessage = 0;
    int connection;

    // get the commit ids
    std::vector<string> commit_ids;
    connection = sqlite3_open("vcs.db", &db);
    int commit_id = 0;
    if (!connection) {
        string query = "SELECT commit_id from commits_to_push;";
        connection = sqlite3_exec(db, query.c_str(), get_files_in_commit, &commit_ids, &errmessage);     
        if( connection != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", errmessage);
                sqlite3_free(errmessage);
        } else {}
    }
    // if nothing to push
    if (commit_ids.size() == 0) {
        cout<<"No commits to push"<<endl;
        return 0;
    }

    connection = sqlite3_open("vcs.db", &db);
    std::vector<string> file_revisions;
    for(int i=0; i<commit_ids.size(); i++){
        string query = "SELECT (file_path) from file_revisions WHERE commit_id = " +  commit_ids[i] + " AND is_removed IS NULL;";
        connection = sqlite3_exec(db, query.c_str(), get_files_in_commit, &file_revisions, &errmessage);
        if (connection != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", errmessage);
            sqlite3_free(errmessage);
        } else {
            // print the files
            for (int i = 0; i < file_revisions.size(); i++) 
                cout<<file_revisions[i]<<endl;
        }
    }
    
    // push to server
    int sock = create_socket_client();
    for(int i = 0; i < file_revisions.size(); i++) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		// keep looping till you get response from server and if first file, then send without getting response 
		cout<<"sending file: "<<file_revisions[i]<<endl;
        if (std::filesystem::is_directory(std::filesystem::path(file_revisions[i]))) {
            send_mkdir_stream(sock, file_revisions[i]);
        } else {
		    send_data(sock, file_revisions[i]);
        }
		// keep receiving until you get response from server
		char response_arr[1024];
		while(1){
			int valread = recv(sock, response_arr, sizeof(response_arr), 0);
			if (valread > 0 )
				break;
			for(int i=0; i<valread; i++)
				cout<<response_arr[i];
			// cout<<"bytes received: "<<valread<<endl;
			printf("response: %s\n", response_arr);
			// in case server closes socket, valread is 0
			if (valread == 0) {
				cout<<"\nServer closed connection";
				exit(EXIT_FAILURE);
			}
		}
    }

    // remove from commits_to_push
    for(int i=0; i<commit_ids.size(); i++){
        connection = sqlite3_open("vcs.db", &db);
        string query = "DELETE FROM commits_to_push WHERE commit_id = " + commit_ids[i] + ";";
        connection = sqlite3_exec(db, query.c_str(), callback, 0, &errmessage);
    }

    cout<<"\u001b[32mPushed successfully to server!\u001b[0m"<<endl;
    return 0;
}


// deprecated as status itself adds the file revisions
// // similar to git add, create temp file to store new files
// int add(string file_path) {
//     create_revisions_directory("revisions");
//     std::vector<string> files_ls;
//     listdir(files_ls);
//     std::vector<string> added_files;
//     get_file_paths(added_files, "added");
//     // cout<<"added files:"<<added_files.size()<<endl;
    
//     // check if file_path entered exists in project dir
//     for (int i=0; i<files_ls.size(); i++){
//         if(strcmp(files_ls[i].c_str(), ("./" + file_path).c_str()) == 0) {
//             if (std::find(added_files.begin(), added_files.end(), "./" + file_path) != added_files.end() != 0) {
//                 cout<<"File already added!"<<endl;
//                 return 1;
//             }
//             std::ofstream added;
//             added.open("revisions/added.txt", std::ios_base::app);
//             added << "./" + file_path<<endl;
//             cout<<"\nAdded file!";
//             return 0;
//         }
//     } 
//     cout <<"\nFile not found!";
//     return 1;
// }


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
    std::vector<std::string> commands {"status", "commit", "push", "remove", "log"};
    // check if empty repository
    create_table();
    create_revisions_directory("revisions");
    if (argc == 1) {
        cout<<"Incorrect usage. Run ./a.out <add/commit/push>"<<endl;
        return 1;
    }

    if (check_args(argv[1], commands)) {
        // if (strcmp(argv[1], "add") == 0)
        //     add(argv[2]);
        if (strcmp(argv[1], "commit") == 0){
            if (argc != 3) {
                cout<<"Invalid format: Enter ./file.out commit <message> "<<endl;
                return -1;
            }
            commit(argv[2]);
        }
        if (strcmp(argv[1], "status") == 0)
            status();
        if (strcmp(argv[1], "log") == 0)
            show_log();
        if (strcmp(argv[1], "push") == 0)
            push_to_server();
    }
    else {
        cout<<"Unknown command: "<<argv[1]<<endl<<"Run ./a.out <add/commit/push>"<<endl;
    }
    // create_file_revision("some");
    return 0;
}
