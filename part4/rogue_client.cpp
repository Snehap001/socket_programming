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
using namespace std;
struct client_config{
    string server_ip;
    int server_port,k,client_id;
};
class RogueClient{

    private:
    const int BUFFSIZE=1024;
    const int NUM_CONC_REQ=5;
    struct client_config config;
    struct sockaddr_in serv_addr;
    char* buffer;
    int communication_socket;
    string incomplete_packet;
    bool file_received;
    std::map<std::string, int> word_count;

    void open_connection();
    void request_contents(int offset);
    void parse_packet(int & words_remaining);
    void read_data();

    public:
    Client(int);
    void load_config();
    void add_time_entry(const string& filename, const vector<string>& new_row);
    void download_file();
    void dump_frequency();
    ~Client();
};
Client::Client(int id){
    buffer=new char[BUFFSIZE];
    config.client_id=id;
}
Client::~Client(){
    delete buffer;
}
void Client::load_config() {

    //loads the configuration of client for communication
    ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration;
    config.server_ip = configuration["server_ip"].get<std::string>();
    config.server_port = configuration["server_port"].get<int>();
    config.k = configuration["k"].get<int>();
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
void Client::request_contents(int offset){

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
    }

}
void Client::parse_packet(int & words_remaining){
    string word=incomplete_packet;
    char* ptr=buffer;
    char ch;
    //reads the buffer till encountering null character
    while((*ptr)!=0){
        ch=*ptr;
        //on reaching end of word, adds it to map
        if(ch==','){
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
            } 
            else {
                word_count.insert({word,1});
            }
            word="";
            ptr++;
            words_remaining--;
            continue;
        }
        //on reaching end of packet, checks if it is invalid/eof
        if(ch=='\n'){
            if((word=="$$")||(word=="EOF")){
                file_received=true;
                words_remaining=-1;
                return;
            }
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
            } 
            else {
                word_count.insert({word,1});
            }
            word="";
            ptr++;
            words_remaining--;           
            continue;          
        }
        word=word+ch;
        ptr++;
    }
    incomplete_packet=word;
}
void Client::download_file() {
    //sets up the connection
    open_connection();
    file_received=false;
    int offset=0;
    //sends requests till the entire file is not received
    while(!file_received){
        int words_remaining=(NUM_CONC_REQ*(config.k));
        for(int i=0;i<NUM_CONC_REQ;i++){
            request_contents(offset);
            offset+=config.k;
        }
        //keeps reading till the entire packet has not been read
        while(words_remaining>0){
            read_data();
            parse_packet(words_remaining);
        }
    }
    //tells the server to close the conversation
    close(communication_socket);
}
void Client::add_time_entry(const string& filename, const vector<string>& new_row) {
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
void Client::dump_frequency(){
    //writes the frequencies to file
    std::ofstream outFile("client_"+to_string(config.client_id)+".txt");
    if (!outFile) {
        std::cerr << "Error opening file for writing!" << std::endl;
    }
    for (const auto& pair : word_count) {
        outFile << pair.first << ", " << pair.second << std::endl;
    }
    outFile.close();
}
int main(int argc, char* argv[]) {
    int id=stoi(argv[1]);
    RogueClient *client=new RogueClient(id);
    client->load_config();
    auto start = chrono::high_resolution_clock::now();
    client->download_file();
    client->dump_frequency();
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    if(argc==3){ 
        if(std::strcmp(argv[2], "plot") == 0){
            vector<string>entry={to_string(id),to_string(duration.count())};
            client->add_time_entry("client_time.csv",entry);
        }
    }
    delete client;
    return 0;
}