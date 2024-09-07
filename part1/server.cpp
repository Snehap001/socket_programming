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
using namespace std;
struct server_config{
    int server_port,k,p;
    const char* fname;
};
class Server{

    private:
    const int BUFFSIZE=1024;   
    const int MAX_CONNECTIONS=1;
    struct server_config config;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    char* buffer;
    string offset;
    int listening_socket,connection_socket;
    vector<string> file;

    void send_file_portion();
    bool parse_request();
    void accept_connection();
    void open_listening_socket();
    bool read_request();

    public:
    Server();
    void load_config();
    void load_data();
    void manage_connection();
    void run();
    ~Server();
};
Server::Server(){
    buffer=new char[BUFFSIZE];
}
Server::~Server(){
    delete buffer;
}
void Server::load_config() {
    //loads the server configuration from the config file
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_port = configuration["server_port"].asInt();
    config.p = configuration["p"].asInt();
    config.k = configuration["k"].asInt();
    config.fname=(configuration["filename"]).asString().c_str();
}
void Server::load_data() {
    //loads the data from the file
    ifstream f(config.fname);
    string word;
    while (getline(f, word, ',')) {
        if (!word.empty()) {
            file.push_back(word);
        }
    }
    file.push_back("EOF");
}
void Server::send_file_portion(){
    int index=stoi(offset);
    int max_index=file.size()-2;
    string packet="";
    if(index>=max_index){
        packet="$$\n";
        send(connection_socket,packet.c_str(), packet.size(), 0);   
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
        send(connection_socket, packet.c_str(), packet.size(), 0);
    }
}
bool Server::read_request(){

    //clears the buffer
    memset(buffer, 0, BUFFSIZE);

    //reads the data from the receive queue into the buffer  
    ssize_t bytes_received = recv(connection_socket, buffer, BUFFSIZE-1,0);
    //if connection has been closed return false
    if(bytes_received == 0){
        return false;
    }
    if (bytes_received < 0) {
        std::cerr << "Read error server" << std::endl;
        close(connection_socket);
        exit(1);
    }
    return true;
}
void Server::accept_connection(){

    // wait for a connection request
    if (listen(listening_socket, MAX_CONNECTIONS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(listening_socket);        
        close(connection_socket);        
        exit(1);
    }    

    //accept request to connect
    socklen_t addrlen=sizeof(clnt_addr);
    connection_socket = accept(listening_socket, (struct sockaddr *)&clnt_addr, &addrlen);
}
void Server::open_listening_socket(){
    // create socket
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(1);
    }  

    int opt=1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //creates the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(config.server_port);

    // bind socket
    if (bind(listening_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(listening_socket);
        exit(1);
    }
}
bool Server::parse_request(){
    char* ptr=buffer;
    //reads the buffer till not encountering a null character
    while((*ptr)!=0){
        char ch=*ptr;
        if(ch=='\n'){
            //reaches end of the packet
            return false;
        }
        offset=offset+ch;
        ptr++;
    }
    //has not reached end of the packet
    return true;
}
void Server::run(){
    //opens the server's socket for listening to connection requests
    open_listening_socket();
    bool connected,request_remaining;
    //listens to connection requests forever
    while(true){
        accept_connection();
        //has connected to the client after accepting connection
        connected=true;
        //entertains all requests while client chooses to remain connected
        while(connected){
            request_remaining=true;
            offset="";
            //reads the request packet entirely
            while(request_remaining){
                connected=read_request();
                if(!connected){
                    break;
                }
                request_remaining=parse_request();
            }
            if(!connected){
                break;
            }
            //sends the file from the requested offset
            send_file_portion();
        }  
        //closes the connection with the client
        close(connection_socket);
    }
}
int main() {
    Server * server=new Server();
    server->load_config();
    server->load_data();
    server->run();
    delete server;
}
