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
#include <pthread.h>
#include "json.hpp"
using json = nlohmann::json;  // Include JSON library
using namespace std;
struct server_config{
    int server_port,k,p,n;
    const char* fname;
};
struct client_data{
    char* buffer;
    string offset;
    int connection_socket;
    ~client_data(){if(buffer){delete buffer;}}
};
class Server{
    private:
    const int BUFFSIZE=1024;   
    const int MAX_CONNECTIONS=1;
    struct server_config config;
    struct sockaddr_in serv_addr;
    int listening_socket;
    vector<string> file;
    pthread_mutex_t connected_locker;
    int client_connected;
    void send_file_portion(client_data* thread_cd);
    bool parse_request(client_data* thread_cd);
    static void* client_handler(void * cd);
    int accept_connection();
    void open_listening_socket();
    bool read_request(client_data* thread_cd);

    public:
    Server();
    void load_config();
    void load_data();
    void run();
    ~Server();
};
struct ThreadArgs{
    struct client_data* cd;
    Server* instance;
    ~ThreadArgs(){if(cd){delete cd;}}
};
Server::Server(){
}
Server::~Server(){
}
void Server::load_config() {
    //loads the server configuration from the config file
    std::ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration;
    config.server_port = configuration["server_port"].get<int>();
    config.p = configuration["p"].get<int>();
    config.k = configuration["k"].get<int>();
    config.fname=configuration["input_file"].get<string>().c_str();
    config.n=configuration["num_clients"].get<int>();
    client_connected=config.n;
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
void Server::send_file_portion(client_data* thread_cd){
    int index=stoi(thread_cd->offset);
    int max_index=file.size()-2;
    string packet="";
    if(index>=max_index){
        packet="$$\n";
        send(thread_cd->connection_socket,packet.c_str(), packet.size(), 0);   
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
        
        send(thread_cd->connection_socket, packet.c_str(), packet.size(), 0);
    }
}
bool Server::read_request(client_data* thread_cd){

    //clears the buffer
    memset(thread_cd->buffer, 0, BUFFSIZE);

    //reads the data from the receive queue into the buffer  
    ssize_t bytes_received = recv(thread_cd->connection_socket, thread_cd->buffer, BUFFSIZE-1,0);
    //if connection has been closed return false
    if(bytes_received == 0){
        return false;
    }
    if (bytes_received < 0) {
        std::cerr << "Read error server" << std::endl;
        close(thread_cd->connection_socket);
        exit(1);
    }
    return true;
}
int Server::accept_connection(){

    // wait for a connection request
    if (listen(listening_socket, MAX_CONNECTIONS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(listening_socket);        
        exit(1);
    }    

    //accept request to connect
    struct sockaddr_in clnt_addr;
    socklen_t addrlen=sizeof(clnt_addr);
    int connection_socket = accept(listening_socket, (struct sockaddr *)&clnt_addr, &addrlen);
    return connection_socket;
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
bool Server::parse_request(client_data* thread_cd){
    char* ptr=thread_cd->buffer;
    //reads the buffer till not encountering a null character
    while((*ptr)!=0){
        char ch=*ptr;
        if(ch=='\n'){
            //reaches end of the packet
            return false;
        }
        thread_cd->offset=thread_cd->offset+ch;
        ptr++;
    }
    //has not reached end of the packet
    return true;
}
void* Server::client_handler(void * args){
    
    pthread_detach(pthread_self());
    //has connected to the client after accepting connection 
    bool connected=true;

    ThreadArgs* thr_args=static_cast<ThreadArgs*>(args);
    client_data* thread_cd = static_cast<client_data*>(thr_args->cd);
    Server* instance = static_cast<Server*>(thr_args->instance);

    //entertains all requests while client chooses to remain connected
    while(connected){
        bool request_remaining=true;
        thread_cd->offset="";
        //reads the request packet entirely
        while(request_remaining){
            connected=instance->read_request(thread_cd);
            if(!connected){
                break;
            }
            request_remaining=instance->parse_request(thread_cd);
        }
        if(!connected){
            break;
        }
        //sends the file from the requested offset
        instance->send_file_portion(thread_cd);
    }  
    //closes the connection with the client
    close(thread_cd->connection_socket);
    pthread_mutex_lock(&(instance->connected_locker));
    instance->client_connected--;
    pthread_mutex_unlock(&(instance->connected_locker));
    delete thr_args;
    return nullptr;
}

void Server::run(){
    //opens the server's socket for listening to connection requests
    open_listening_socket();
    //listens to connection requests forever
    int num_clients_connected=config.n;
    while(num_clients_connected>0){
        int connection_socket=accept_connection(); 
        num_clients_connected--;
        char* buffer=new char[BUFFSIZE];
        client_data* cd = new client_data{buffer,"",connection_socket}; 
        ThreadArgs* args=new ThreadArgs{cd,this};
        pthread_t thread; 
        if (pthread_create(&thread, nullptr, client_handler, args) != 0) {
            cerr << "Error creating thread" << endl;
            close(connection_socket);
            continue;
        }
    }
    bool keep_running=true;
    while(keep_running){
        pthread_mutex_lock(&(connected_locker));
        keep_running=client_connected>0;
        pthread_mutex_unlock(&(connected_locker));

    }    
    close(listening_socket);
    
}

int main() {
    Server * server=new Server();
    server->load_config();
    server->load_data();
    server->run();
    delete server;
}