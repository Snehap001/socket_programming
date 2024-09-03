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
#include <jsoncpp/json/json.h>
#include <math.h>
#include <numeric>
using namespace std;
struct client_config{
    string server_ip;
    int server_port;
};
class Client{

    private:
    const int BUFFSIZE=1024;
    struct client_config config;
    struct sockaddr_in serv_addr;
    char* buffer;
    int communication_socket;
    string incomplete_packet;
    map<string, int> word_count;
  

    void open_connection();
    void send_request(int offset);
    bool parse_message();
    void read_data();

    public:
    Client();
    void load_config();
    void manage_connection();
    void dump_frequency();
    ~Client();
};
Client::Client(){
    buffer=new char[BUFFSIZE];
    incomplete_packet="";
}
Client::~Client(){
    delete buffer;
}
void Client::load_config() {

    //loads the configuration of client for communication
    ifstream config_file("config.json", ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_ip = configuration["server_ip"].asString();
    config.server_port = configuration["server_port"].asInt();
    config_file.close();

}
void Client::open_connection() {

    //creates the socket for the data connection
    if ((communication_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Socket creation error" << endl;
    }

    //creates the server adddress
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(config.server_port);
    if (inet_pton(AF_INET, config.server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        close(communication_socket);
    }

    //does the three-way handshake
    if (connect(communication_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed" << endl;
        close(communication_socket);
    }

}
void Client::send_request(int offset){

    //sends the request containing the offset
    string request = (to_string(offset)+"\n");
    send(communication_socket, request.c_str(), request.size(), 0);
    incomplete_packet="";

}
void Client::read_data(){

    //clears the buffer
    memset(buffer, 0, BUFFSIZE);

    //reads the data from the receive queue into the buffer  
    int bytes_received = recv(communication_socket, buffer, BUFFSIZE-1,0);
    if (bytes_received < 0) {
        cerr << "Read error client" << endl;
        close(communication_socket);
       
        
    }
    
    // cout << "Received: " << bytes_received << endl; 
   

}
bool Client::parse_message(){
    string word=incomplete_packet;
    for(int i=0;i<BUFFSIZE;i++){
        char ch=buffer[i];
        if(ch==0){
            break;
        }
        if(ch==','){
            if(word=="EOF"){
                return false;
            }
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
            } 
            else {
                word_count.insert({word,1});
            }
            word="";
            continue;
        }
        if(ch=='\n'){
            if(word=="EOF"){
                return false;
            }
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
            } 
            else {
                word_count.insert({word,1});
            }
            word="";            
        }
        word=word+ch;
    }
    incomplete_packet=word;
    return true;
}
void Client::manage_connection() {
    open_connection();
    send_request(10);
    bool data_remaining=true;
    while(data_remaining){
        read_data();
        data_remaining=parse_message();
        

    }
    close(communication_socket);
}
void Client::dump_frequency(){
    ofstream outFile("output.txt");
    if (!outFile) {
        cerr << "Error opening file for writing!" << endl;
    }
    for (const auto& pair : word_count) {
        outFile << pair.first << ", " << pair.second << endl;
    }
    outFile.close();
}
void updateConfig(int p) {
    ifstream config_file("config.json", ifstream::binary);
    Json::Value config;
    config_file >> config;
    config_file.close();

    // Update the "p" value
    config["p"] = p;
    // Write back the updated config to the file
    ofstream updated_config_file("config.json");
    updated_config_file << config;
    updated_config_file.close();
}
double measureCompletionTime(){
    auto start = chrono::high_resolution_clock::now();
    Client *client=new Client();
    client->load_config();
    client->manage_connection();
    client->dump_frequency();
    delete client;
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    return duration.count(); 
}
double computeStdDev(const vector<double>& times, double mean) {
    double sum = 0.0;
    for (double time : times) {
        sum += (time - mean) * (time - mean);
    }
    return sqrt(sum / times.size());
}
int main() {
    const int NUM_RUNS = 10;
    vector<double> average_times;
    vector<double> confidence_intervals;

    for (int p = 1; p <= 10; ++p) {
        vector<double> times;

        for (int i = 0; i < NUM_RUNS; ++i) {
            updateConfig(p);  
            double time = measureCompletionTime();
            times.push_back(time);
        }

        double mean = accumulate(times.begin(), times.end(), 0.0) / times.size();
        average_times.push_back(mean);

        // Calculate the 95% confidence interval (approx. 1.96 * standard deviation)
        double stddev = computeStdDev(times, mean);
        double confidence_interval = 1.96 * stddev / sqrt(NUM_RUNS);  // 95% CI
        confidence_intervals.push_back(confidence_interval);

        // Optionally, save the results to a file
        ofstream outFile("results_p" + to_string(p) + ".txt");
        for (double time : times) {
            outFile << time << endl;
        }
        outFile.close();
    }

    // Save the average times and confidence intervals to a file for plotting
    ofstream resultsFile("average_times_with_ci.txt");
    for (int p = 1; p <= 10; ++p) {
        resultsFile << p << ", " << average_times[p - 1] << ", " << confidence_intervals[p - 1] << endl;
    }
    resultsFile.close();

    return 0;

    return 0;
}