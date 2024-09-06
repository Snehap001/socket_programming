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

class Experiment{
    public:
    Json::Value config;
    Experiment(){
        ifstream config_file("config.json", ifstream::binary);
        
        config_file >> config;
        config_file.close();
    }
    void updateConfig(int n);
    void execute();
    void run();

    ~Experiment(){

    }
    
};

void Experiment::updateConfig(int n) {

    config["n"] = n;
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
void run_multi_client(){
    int status=system("./client");
    if(status!=0){
        cout<<"failed client"<<endl;
    }
    
}
void Experiment::execute(){
    run_multi_client();
    


}

void add_entry(const string& filename, const vector<string>& new_row) {
    ofstream file(filename, ios::app);


    for (size_t i = 0; i < new_row.size(); ++i) {
        file << new_row[i];
        if (i != new_row.size() - 1) {
            file << ",";  
        }
    }
    file << "\n"; 

    file.close();
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
        string id_str, time_str;

        getline(ss, id_str, ',');
        getline(ss, time_str, ',');

        double time = stod(time_str);
        times.push_back(time);
        
    }

    csv_file.close();

    double sum = accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

void write_average_to_file(const string& txt_filename, double average, int n) {
    ofstream txt_file(txt_filename, ios::app);
    if (!txt_file.is_open()) {
        cerr << "Could not open the text file!" << endl;
        return;
    }

    txt_file << n << " "<<average<<endl;


    txt_file.close();
}
void Experiment:: run(){
    
    int max_n=32;
  
    vector<double> average_times;
    vector<double> confidence_intervals;
    for (int n=1;n<=max_n;n=n+4){
        
        string filename="client_time.csv";
        ofstream file(filename, ios::out);
        file.close();
        updateConfig(n);
        vector<string>entry={"id","time"};
        add_entry("client_time.csv",entry);
        execute();
        double avg=calculate_average_time(filename);
        write_average_to_file("avg_time_per_client.txt",avg,n);
    }
    

}

int main(){
    Experiment E;
    E.run();
    return 0;
}  