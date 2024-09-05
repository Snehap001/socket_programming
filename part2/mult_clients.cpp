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
    int status=system("./server &");
    if(status!=0){
        cout<<"failed server"<<endl;
    }
}
void run_clients(int num_clients){
    
    for (int i=1;i<=num_clients;i++){
        string input_str="./client "+to_string(i);
        const char *c=input_str.c_str();
        int status=system(c);
        if(status!=0){
            cout<<"client failed"<<endl;
        }

    }
    
}
void MultClients::execute(){
    run_server;
    run_clients(num_clients);

}
int main(){
    MultClients M;
    M.execute();
}
