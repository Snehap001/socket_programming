#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <vector>
#include <chrono>
#include <thread> 

#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <math.h>
#include <numeric>
using namespace std;
class MultClients{
    public:
    int num_clients;
    MultClients();
    void execute();

};
MultClients::MultClients(){
    ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    
    num_clients = configuration["n"].asInt();
    config_file.close();
}
void run_server(){
    int status=system("./server > server_output.log 2>&1 &");
    if(status!=0){
        cout<<"failed server"<<endl;
    }
}
void run_executable(string command) {
    system(command.c_str());
}
void run_clients(int num_clients){
    
    vector<string> commands;
    for (int i=1;i<=num_clients;i++){
        string s="./client "+to_string(i);
        commands.push_back(s);
    }

    vector<thread> threads;

    for (auto cmd : commands) {
        
        threads.emplace_back(run_executable,cmd);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Join all threads to ensure they finish
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    
}
void MultClients::execute(){
    run_server;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    run_clients(num_clients);

}
int main(){
    MultClients M;
    M.execute();
}
