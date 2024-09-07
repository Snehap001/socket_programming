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
    int server_port,k,client_id;
};
class Client{

    private:
    const int BUFFSIZE=1024;
    struct client_config config;
    struct sockaddr_in serv_addr;
    char* buffer;
    int communication_socket;
    string incomplete_packet;
    bool file_received;
    std::map<std::string, int> word_count;

    void open_connection();
    void request_contents(int offset);
    bool parse_packet();
    void read_data();

    public:
    Client(int);
    void load_config();
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
bool Client::parse_packet(){
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
            continue;
        }
        //on reaching end of packet, checks if it is invalid/eof
        if(ch=='\n'){
            if((word=="$$")||(word=="EOF")){
                file_received=true;
                return false;
            }
            auto it = word_count.find(word);
            if (it != word_count.end()) {
                word_count[word]++;
            } 
            else {
                word_count.insert({word,1});
            }
            //indicates that the entire packet has been read
            return false;           
        }
        word=word+ch;
        ptr++;
    }
    incomplete_packet=word;
    return true;
}
void Client::download_file() {
    //sets up the connection
    open_connection();
    file_received=false;
    int offset=0;
    //sends requests till the entire file is not received
    while(!file_received){
        request_contents(offset);
        bool data_remaining=true;
        //keeps reading till the entire packet has not been read
        while(data_remaining){
            read_data();
            data_remaining=parse_packet();
        }
        offset+=config.k;
    }
    //tells the server to close the conversation
    close(communication_socket);
}
void Client::dump_frequency(){
    //writes the frequencies to file
    std::ofstream outFile("output_"+to_string(config.client_id)+".txt");
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
    Client *client=new Client(id);
    client->load_config();
    client->download_file();
    client->dump_frequency();
    delete client;
    return 0;
}