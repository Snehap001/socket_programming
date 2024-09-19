#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <vector>
#include <chrono> 
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"
using json = nlohmann::json;
#include <math.h>
#include <numeric>
using namespace std;

class Experiment{
    private:
    json config;
    void updateConfig(int n);
    void execute(int prot_num);
    void run_server();

    void run_multi_client(int prot_num);
    void add_entry(const string& filename, const vector<string>& new_row);
    void write_average_to_file(const string& txt_filename, double average, int n);
    double calculate_average_time(const string& csv_filename);
    public:
    Experiment(){
        ifstream config_file("config.json", ifstream::binary);
        config_file >> config;
        config_file.close();
    }
    void run(int prot_num);
    void combine();
    ~Experiment(){}   
};

void Experiment::updateConfig(int n) {
    config["num_clients"] = n;
    ofstream updated_config_file("config.json");
    updated_config_file << config;
    updated_config_file.close();
}
void Experiment::run_server(){
    int status=system("./server &");
    if(status!=0){
        cout<<"failed server"<<endl;
    }

}

void Experiment::run_multi_client(int prot_num){
    if(prot_num==0){
        int status=system("./client aloha plot");
        if(status!=0){
            cout<<"failed client"<<endl;
        }
    }
    else if(prot_num==1){
        int status=system("./client beb plot");
        if(status!=0){
            cout<<"failed client"<<endl;
        }
    }
    else{
        int status=system("./client cscd plot");
        if(status!=0){
            cout<<"failed client"<<endl;
        }
    }
    
}

void Experiment::add_entry(const string& filename, const vector<string>& new_row) {
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
void Experiment::execute(int prot_num){
    run_server();
 
    run_multi_client(prot_num);
}
double Experiment::calculate_average_time(const string& csv_filename) {
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

        getline(ss, time_str, ',');

        double time = stod(time_str);
        times.push_back(time);
        
    }
    csv_file.close();
    double sum = accumulate(times.begin(), times.end(), 0.0);
    return sum / times.size();
}

void Experiment::write_average_to_file(const string& txt_filename, double average, int n) {
    ofstream txt_file(txt_filename, ios::app);
    if (!txt_file.is_open()) {
        cerr << "Could not open the text file!" << endl;
        return;
    }
    txt_file << n << " "<<average<<endl;
    txt_file.close();
}
void Experiment:: run(int prot_num){   
    string prot="";
    if(prot_num==0){
        prot="aloha";
    }
    else if (prot_num==1){
        prot="beb";
    }
    else{
        prot="cscd";
    }
    
    int max_n=32;

    ofstream time_file("avg_time_"+prot+".txt", ios::out);
    time_file.close();
    for (int n=1;n<=max_n;n=n+4){     
        sleep(1);  
        string filename="client_time.csv";
        ofstream file(filename, ios::out);
        file.close();
        updateConfig(n);
    
        execute(prot_num);
        double avg=calculate_average_time(filename);
        write_average_to_file("avg_time_"+prot+".txt",avg,n);
    }
}

int main(){
    Experiment E;
    E.run(0);
    E.run(1);
    E.run(2);
  
    return 0;
}  