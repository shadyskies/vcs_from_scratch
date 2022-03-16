#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <filesystem>
 
using namespace std::chrono_literals;
int main()
{
    auto p = std::filesystem::temp_directory_path() / "example.bin";
    std::ofstream(p.c_str()).put('a'); // create file
 
    auto print_last_write_time = [](std::filesystem::file_time_type const& ftime) {
        std::time_t cftime = std::chrono::system_clock::to_time_t(
            std::chrono::file_clock::to_sys(ftime));
        std::cout << "File write time is " << std::asctime(std::localtime(&cftime));
    };
 
    auto ftime = std::filesystem::last_write_time(p);
    print_last_write_time(ftime);
 
    std::filesystem::last_write_time(p, ftime + 1h); // move file write time 1 hour to the future
    ftime = std::filesystem::last_write_time(p); // read back from the filesystem
    print_last_write_time(ftime);
 
    std::filesystem::remove(p);
}