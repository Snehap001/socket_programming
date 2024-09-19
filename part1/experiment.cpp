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

#include <math.h>
#include <numeric>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

class Experiment{
    public:
    json config;
    Experiment(){
        ifstream config_file("config.json", ifstream::binary);
        
        config_file >> config;
        config_file.close();
    }
    void updateConfig(int p);
    void execute();
    void run();

    ~Experiment(){

    }
    
};

void Experiment::updateConfig(int p) {

    config["p"] = p;
    ofstream updated_config_file("config.json");
    updated_config_file << config;
    updated_config_file.close();
}
void run_server(){
    int status=system("./server &");
    if(status!=0){
        cout<<"failed server"<<endl;
    }
    else{
        cout<<"server started"<<endl;
    }

}
void run_client(){
    int status=system("./client");
    if(status!=0){
        cout<<"failed client"<<endl;
    }
    else{
        cout<<"client started"<<endl;
    }

}
void Experiment::execute(){
    run_server();
    run_client();


}
double computeStdDev(const vector<double>& times, double mean) {
    double sum = 0.0;
    for (double time : times) {
        sum += (time - mean) * (time - mean);
    }
    return sqrt(sum / times.size());
}
void Experiment:: run(){
    
    int K=10;
    int NUM_RUNS=10;
    vector<double> average_times;
    vector<double> confidence_intervals;
    for (int p=1;p<=K;p++){
        vector<double> times;

        for (int i = 0; i < NUM_RUNS; ++i) {
            sleep(1);
            updateConfig(p);
            auto start = chrono::high_resolution_clock::now();
            execute();
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double> duration = end - start;
            times.push_back(duration.count());

        }
        double mean = accumulate(times.begin(), times.end(), 0.0) / times.size();
        average_times.push_back(mean);

        double stddev = computeStdDev(times, mean);
        double confidence_interval = 1.96 * stddev / sqrt(NUM_RUNS);  // 95% CI
        confidence_intervals.push_back(confidence_interval);

        
    }
    ofstream resultsFile("average_times_with_ci.txt");
    for (int p = 1; p <= K; ++p) {
        resultsFile << p << ", " << average_times[p - 1] << ", " << confidence_intervals[p - 1] << endl;
    }
    resultsFile.close();


}

int main(){
    Experiment E;
    E.run();
    return 0;
}  