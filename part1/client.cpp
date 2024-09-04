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

    ifstream config_file("config.json", ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_ip = configuration["server_ip"].asString();
    config.server_port = configuration["server_port"].asInt();
 
    config_file.close();

}
void Client::open_connection() {

    if ((communication_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Socket creation error" << endl;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(config.server_port);
    if (inet_pton(AF_INET, config.server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        close(communication_socket);
    }

    if (connect(communication_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed" << endl;
        close(communication_socket);
    }

}
void Client::send_request(int offset){

    string request = (to_string(offset)+"\n");
    send(communication_socket, request.c_str(), request.size(), 0);
    incomplete_packet="";

}
void Client::read_data(){

    memset(buffer, 0, BUFFSIZE);

    int bytes_received = recv(communication_socket, buffer, BUFFSIZE-1,0);
    if (bytes_received < 0) {
        close(communication_socket);
       
        
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
            continue;          
        }
       
        word=word+ch;
        
        
    }
    incomplete_packet=word;
    if(incomplete_packet=="\n" || incomplete_packet=="")return false;
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


int main(int argc, char* argv[]) {
    Client *client=new Client();
    client->load_config();
    client->manage_connection();
    client->dump_frequency();
    delete client;
    return 0;
}