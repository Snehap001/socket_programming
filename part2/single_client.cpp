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
using namespace std;
struct client_config{
    string server_ip;
    int server_port,k;
};
class Client{

    private:
    const int BUFFSIZE=1024;
    struct client_config config;
    struct sockaddr_in serv_addr;
    char* buffer;
    int communication_socket;
    string incomplete_packet;
    int counter;
    std::map<std::string, int> word_count;
    int client_id;

    void open_connection();
    void send_request(int offset);
    bool parse_message();
    void read_data();

    public:

    Client(int id);
    void load_config();
    void manage_connection();
    void dump_frequency();
    ~Client();
};
Client::Client(int id){
    buffer=new char[BUFFSIZE];
    incomplete_packet="";
    client_id=id;
}
Client::~Client(){
    delete buffer;
}
void Client::load_config() {

    //loads the configuration of client for communication
    ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_ip = configuration["server_ip"].asString();
    config.server_port = configuration["server_port"].asInt();
    config.k = configuration["k"].asInt();
    config_file.close();

}
void Client::open_connection() {

    //creates the socket for the data connection
    if ((communication_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
    }

    //creates the server adddress
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(config.server_port);
    if (inet_pton(AF_INET, config.server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        close(communication_socket);
    }

    //does the three-way handshake
    if (connect(communication_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
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
        std::cerr << "Read error client" << std::endl;
        close(communication_socket);
        exit(1);
    }

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
                counter++;
            } 
            else {
                word_count.insert({word,1});
                counter++;
            }
            if(counter==config.k){
                return false;
            }
            word="";
            continue;
        }
        if(ch=='\n'){
            if(word=="$$"){
                return false;
            }
            if(word=="EOF"){
                return false;
            }
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
                counter++;
            } 
            else {
                word_count.insert({word,1});
                counter++;
            }
            if(counter==config.k){
                return false;
            }
            word=""; 
            continue;           
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
    counter=0;
    while(data_remaining){
        read_data();
        data_remaining=parse_message();

    }
    close(communication_socket);
}
void Client::dump_frequency(){
    std::ofstream outFile("outputs/output_"+to_string(client_id)+".txt");
    if (!outFile) {
        std::cerr << "Error opening file for writing!" << std::endl;
    }
    for (const auto& pair : word_count) {
        outFile << pair.first << ", " << pair.second << std::endl;
    }
    outFile.close();
}
void add_time_entry(const string& filename, const vector<string>& new_row) {
    ofstream file(filename, ios::app);

    if (!file.is_open()) {
        cerr << "Could not open the file!" << std::endl;
        return;
    }

    for (size_t i = 0; i < new_row.size(); ++i) {
        file << new_row[i];
        if (i != new_row.size() - 1) {
            file << ",";  
        }
    }
    file << "\n"; 

    file.close();
}
int main(int argc, char* argv[]) {
    int id=stoi(argv[1]);

    Client *client=new Client(id);

    client->load_config();
    auto start = chrono::high_resolution_clock::now();
    client->manage_connection();

    client->dump_frequency();
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    vector<string>entry={to_string(id),to_string(duration.count())};
    add_time_entry("client_time.csv",entry);
    delete client;
    return 0;
}