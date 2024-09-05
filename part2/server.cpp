#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <jsoncpp/json/json.h>  // Include JSON library
#include <thread>
using namespace std;
struct server_config{
    int server_port,k,p,n;
    string offset;
};
class Server{

    private:

    const int BUFFSIZE=1024;   
    int MAX_CONNECTIONS=5;
    struct server_config config;
    struct sockaddr_in serv_addr;

    char* buffer;
    int server_socket;
    vector<string> file;
    vector<thread> threads;

    void send_file(int client_socket);
    bool parse_request();
    int accept_connection();
    void open_socket();
    void read_request(int client_socket);
    void create_threads();

    public:
    Server();
    void load_config();
    void load_data(const char*);
    void manage_connection(int client_socket);
    void connect();
    ~Server();
};
Server::Server(){
    config.offset="";
    buffer=new char[BUFFSIZE];
    MAX_CONNECTIONS=config.n;
}
Server::~Server(){
    delete [] buffer;
}
void Server::load_config() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_port = configuration["server_port"].asInt();
    config.p = configuration["p"].asInt();
    config.k = configuration["k"].asInt();
    config.n=configuration["n"].asInt();
}
void Server::load_data(const char* fname) {
    ifstream f(fname);
    string word;
    while (getline(f, word, ',')) {
        if (!word.empty()) {
            file.push_back(word);
        }
    }
    file.push_back("EOF");
}
void Server::send_file(int client_socket){
    int index=stoi(config.offset);
    int max_index=file.size()-2;
    string packet="";
    if(index>=max_index){
        packet="$$\n";
        send(client_socket,packet.c_str(), packet.size(), 0);   
        return;
    }
    int num_words=config.k;
    int packet_size=config.p;
    int last_index=index+num_words-1;
    if(last_index>=max_index){
        last_index=file.size()-1;
    }
    while(index<=last_index){
        packet="";
        int this_packet_size=0;
        while(this_packet_size<packet_size){
            if(index>last_index){
                break;
            }
            packet=packet+file[index]+",";
            this_packet_size++;
            index++;
        }
        packet.pop_back();
        packet=packet+"\n";
        send(client_socket, packet.c_str(), packet.size(), 0);
    }
}
void Server::read_request(int client_socket){

    //clears the buffer
    memset(buffer, 0, BUFFSIZE);

    //reads the data from the receive queue into the buffer  
    ssize_t bytes_received = recv(client_socket, buffer, BUFFSIZE-1,0);
    if (bytes_received < 0) {
        std::cerr << "Read error server" << std::endl;
        close(server_socket);
        close(client_socket);
        exit(1);
    }
}
void Server::manage_connection(int client_socket){

    
    bool request_remaining=true;
    while(request_remaining){
        read_request(client_socket);
        request_remaining=parse_request();
    }
    send_file(client_socket);
        
    close(client_socket);
    
}
int Server::accept_connection(){
    struct sockaddr_in clnt_addr;
    // wait for a connection request
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_socket);        
          
        exit(1);
    }    

    //accept request to connect
    socklen_t addrlen=sizeof(clnt_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&clnt_addr, &addrlen);
    config.offset="";
    return client_socket;
}
void Server::open_socket(){
    // create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(1);
    }  

    int opt=1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //creates the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(config.server_port);

    // bind socket
    if (bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }

}
void Server::create_threads(){
  
    while(true){
        
        int client_socket = accept_connection();
        
        threads.push_back(std::thread([this, client_socket]() {manage_connection(client_socket);}));
        threads.back().detach();
    }
    
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }


}
void Server::connect(){
    open_socket();
    create_threads();
    close(server_socket);
}
bool Server::parse_request(){
    for(int i=0;i<BUFFSIZE-1;i++){
        char ch=buffer[i];
        if(ch=='\n'){
            return false;
        }
        config.offset=config.offset+ch;
    }
    return true;
}
int main() {
    Server * server=new Server();
    server->load_config();
    server->load_data("word.txt");
    server->connect();
    delete server;
}
