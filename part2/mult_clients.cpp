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
public:
    int num_clients;
    MultClients();
    void execute();
};

MultClients::MultClients() {
    ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;

    num_clients = configuration["n"].asInt();
    config_file.close();
}

void* run_server(void* arg) {
    int status = system("./server &");
    if (status != 0) {
        cout << "Failed to start server" << endl;
    } else {
        cout << "Server started" << endl;
    }
    return nullptr;
}

void* run_executable(void* arg) {
    string command = *(static_cast<string*>(arg));
    int status = system(command.c_str());
    if (status != 0) {
        cout << "Failed: " << command << endl;
    } else {
        cout << "Successful: " << command << endl;
    }
    return nullptr;
}

void run_clients(int num_clients) {
    vector<string> commands;
    for (int i = 1; i <= num_clients; i++) {
        string s = "./client " + to_string(i);
        commands.push_back(s);
    }

    vector<pthread_t> threads(num_clients);

    for (int i = 0; i < num_clients; i++) {
        pthread_create(&threads[i], nullptr, run_executable, static_cast<void*>(&commands[i]));
        sleep(1);  // Sleep for 1 second to simulate staggered client start
    }

    // Join all threads to ensure they finish
    for (auto& th : threads) {
        pthread_join(th, nullptr);
    }
}

void MultClients::execute() {
    pthread_t server_thread;

    // Start the server in a separate thread
    pthread_create(&server_thread, nullptr, run_server, nullptr);
    sleep(3);  // Sleep for 3 seconds to give the server time to start

    // Start the clients
    run_clients(num_clients);

    // Join the server thread (optional, depending on whether you want to wait for server shutdown)
    pthread_join(server_thread, nullptr);
}

int main() {
    MultClients M;
    M.execute();
    return 0;
}
