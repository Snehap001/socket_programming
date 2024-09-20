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
#include "json.hpp"
using json = nlohmann::json;
#include <math.h>
#include <numeric>

using namespace std;

class MultClients {
    private:
    int num_clients;
    static void* run_executable(void* arg);
    public:
    string arg;
    MultClients();
    void run_clients();
    void run_fairness();
};

MultClients::MultClients() {
    ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration;

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
        cout<<"success"<<command<<endl;
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
void MultClients::run_fairness() {
    vector<string> commands;
    string s = "./rogue_client 1 " +arg;
    commands.push_back(s);
    for (int i = 2; i <= num_clients; i++) {
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
double calc_fairness_index(string csv_filename,int n){
    ifstream csv_file(csv_filename);

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

    double sum_val=0.0;
    double square_sum=0.0;
    for (auto v:times){
        sum_val+=1/v;
        square_sum+=(1/(v*v));
    }
    double index=(sum_val*sum_val)/(n*square_sum);
    return index;
    
}
int main(int argc, char* argv[]) {
    MultClients M;
    if(argc==3){
        if(std::strcmp(argv[1], "plot") == 0  ){
            string schedule=argv[2];
            M.arg="plot "+schedule;
            M.run_clients();
        }
        else if(strcmp(argv[1],"fairness")==0){

            string schedule=argv[2];
            M.arg="not_plot "+schedule;
            M.run_fairness();
            double avg_time=calculate_average_time("client_time_"+schedule+".csv");
            double index=calc_fairness_index("client_time_"+schedule+".csv",10);
            ofstream txt_file("jains_fairness_index.txt", ios::app);
            txt_file<<avg_time<<","<<index<<endl;
            txt_file.close();

        }
        else{
            M.arg="not_plot";
        }
    }
    else if(argc==2){
        string schedule=argv[1];
        M.arg="not_plot "+schedule; 
        M.run_clients();
        double avg_time=calculate_average_time("client_time_"+schedule+".csv");
        cout<<avg_time<<endl;
    }
   
    return 0;
}