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

#include <math.h>
#include <numeric>
#include "json.hpp"
using json = nlohmann::json;

using namespace std;

class MultClients {
    private:
    int num_clients;
    static void* run_executable(void* arg);
    public:
    string arg;
    MultClients();
    void run_clients();
};

MultClients::MultClients() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration; // Parse the JSON content from the file

    num_clients = configuration["num_clients"].get<int>();

    config_file.close(); 


}
void* MultClients::run_executable(void* arg) {
    string command = *(static_cast<string*>(arg));
    int status = system(command.c_str());
    if (status != 0) {
        cout << "Failed: " << command << endl;
    } 
    else{
        cout<<"success: "<<command<<endl;
    }
    return nullptr;
}

void MultClients::run_clients() {
    vector<string> commands;
    for (int i = 1; i <= num_clients; i++) {
        string s = "./single_client " + to_string(i)+" "+arg;
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
double calculate_average_time(const string& csv_filename) {
    ifstream csv_file(csv_filename);
    if (!csv_file.is_open()) {
        cerr << "Could not open the CSV file!" << endl;
        return -1;
    }
    string line;
    vector<double> times;
    getline(csv_file, line);
    while (getline(csv_file, line)) {
        stringstream ss(line);
        string time_str;

     
        getline(ss, time_str);

        double time = stod(time_str);
        times.push_back(time);
        
    }
    csv_file.close();
    double sum = accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

int main(int argc, char* argv[]) {
    MultClients M;
    string prot=argv[1];
    if(prot=="all"){
        string filename="client_time.csv";
        ofstream file1(filename, ios::out);
        file1.close();
        M.arg="aloha all";
        M.run_clients();
        double slotted_time=calculate_average_time(filename);
        ofstream file2(filename, ios::out);
        file2.close();
        M.arg="beb all";
        M.run_clients();
        double beb_time=calculate_average_time(filename);
        ofstream file3(filename, ios::out);
        file3.close();
        
        M.arg="cscd all";
        M.run_clients();
        double cscd_time=calculate_average_time(filename);
        ofstream txt_file("avg_time.txt", ios::app);
        txt_file <<slotted_time<<","<<beb_time<<","<<cscd_time<<endl;
        txt_file.close();
        return 0;
    }
    else if(argc==3){
        if(std::strcmp(argv[2], "plot") == 0){
            M.arg=prot+" plot";
            M.run_clients();
        }
        else{
            M.arg=prot+" not_plot";
            ofstream file3("client_time.csv", ios::out);
            file3.close();
            M.run_clients();
            
            double avg_time=calculate_average_time("client_time.csv");
            cout<<avg_time<<endl;
        }
    }
    else{
        M.arg=prot+" not_plot"; 
        ofstream file3("client_time.csv", ios::out);
        file3.close();
        M.run_clients();

        double avg_time=calculate_average_time("client_time.csv");
        cout<<avg_time<<endl;       
    }
    
    return 0;
}
