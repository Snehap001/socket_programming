#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <queue>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <map>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;
int findPort() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration; // Parse the JSON content from the file
    int server_port = configuration["server_port"].get<int>();
    return server_port;
}
void killProcessesOnPort(int port) {
    
    std::string command = "lsof -t -i:" + std::to_string(port) + " | xargs kill -9";
    int ret = system(command.c_str());
    
    if (ret == -1) {
        std::cerr << "Error executing command to kill processes on port " << port << ": " << strerror(errno) << std::endl;
    } else {
        std::cout << "Killed processes on port " << port << std::endl;
    }
}
int main(){
    int port=findPort();
    killProcessesOnPort(port);
}