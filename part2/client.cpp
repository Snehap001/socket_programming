#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <vector>
#include <chrono>
#include <pthread.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <math.h>
#include <numeric>

using namespace std;

class MultClients {
    private:
    int num_clients;
    static void* run_executable(void* arg);
    public:

    MultClients();
    void run_clients();
};

MultClients::MultClients() {
    ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;

    num_clients = configuration["n"].asInt();
    config_file.close();
}
void* MultClients::run_executable(void* arg) {
    string command = *(static_cast<string*>(arg));
    int status = system(command.c_str());
    if (status != 0) {
        cout << "Failed: " << command << endl;
    } 
    return nullptr;
}

void MultClients::run_clients() {
    vector<string> commands;
    for (int i = 1; i <= num_clients; i++) {
        string s = "./single_client " + to_string(i);
        commands.push_back(s);
    }

    vector<pthread_t> threads(num_clients);

    for (int i = 0; i < num_clients; i++) {
        pthread_create(&threads[i], nullptr, run_executable, static_cast<void*>(&commands[i]));
    }

    for (auto& th : threads) {
        pthread_join(th, nullptr);
    }
}

int main() {
    MultClients M;
    M.run_clients();
    return 0;
}
