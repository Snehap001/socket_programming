#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <queue>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <map>
#include <arpa/inet.h>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;
enum policy{fifo,round_robin};
struct server_config{
    int server_port,k,p,n;
    string fname;
    policy pol;
    string server_ip;
};
struct client_data{
    char* buffer;
    string offset;
    int connection_socket;
    ~client_data(){if(buffer){delete buffer;}}
};
struct request{
    int connection_socket;
    int offset;
};
class Server{
    private:
    const int BUFFSIZE=1024;   
    const int MAX_CONNECTIONS=1;
    struct server_config config;
    struct sockaddr_in serv_addr;
    int listening_socket;
    vector<string> file;
    queue<request> fifo_queue;
    pthread_mutex_t queue_locker;
    map<int,queue<int>>rr_map;
    pthread_mutex_t map_locker;
    vector<int>rr_classes;
    pthread_mutex_t rr_class_locker;

    pthread_mutex_t connected_locker;
    int client_connected=0;
    void send_file_portion(request r);
    void parse_offset(client_data* thread_cd,queue<int>&offsets);
    void parse_request(client_data* thread_cd,queue<request>&requests);
    int accept_connection();
    void open_listening_socket();
    bool read_request(client_data* thread_cd);
    static void* fifo_queue_handler(void * args);
    static void* fifo_client_handler(void * cd);
    static void* rr_map_handler(void* args);
    static void* rr_client_handler(void* cd);

    public:
    Server();
    void set_policy(string policy_choice);
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
void Server::set_policy(string policy_choice){
    if(policy_choice=="fifo"){
        config.pol=fifo;
    }
    else if(policy_choice=="rr"){
        config.pol=round_robin;
        rr_classes.resize(config.n, -1);
    }
}
void Server::load_config() {
    //loads the server configuration from the config file
    std::ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration; // Parse the JSON content from the file
    config.server_port = configuration["server_port"].get<int>();
    config.p = configuration["p"].get<int>();
    config.k = configuration["k"].get<int>();
    config.n = configuration["num_clients"].get<int>();
    config.fname=configuration["input_file"].get<std::string>();
    config.server_ip=configuration["server_ip"].get<std::string>();
    client_connected=config.n;
}
void Server::load_data() {
    //loads the data from the file
    ifstream f(config.fname.c_str());
    string word;
    while (getline(f, word, ',')) {
        if (!word.empty()) {
            file.push_back(word);
        }
    }
    file.push_back("EOF");
}
void Server::send_file_portion(request r){
    int index=r.offset;
    int max_index=file.size()-2;
    string packet="";if(index>=max_index){
        packet="$$\n";
        send(r.connection_socket,packet.c_str(), packet.size(), 0);   
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
        send(r.connection_socket, packet.c_str(), packet.size(), 0);
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
    

    //accept request to connect
    struct sockaddr_in clnt_addr;
    socklen_t addrlen=sizeof(clnt_addr);
    int connection_socket = accept(listening_socket, (struct sockaddr *)&clnt_addr, &addrlen);
    return connection_socket;
}
void* Server::rr_map_handler(void * args){
    pthread_detach(pthread_self()); 
    Server* instance=static_cast<Server*>(args);
    int class_index=0;
    int num_connections=instance->config.n;
    int connection_socket;
    bool keep_running=true;
    while(keep_running){
        pthread_mutex_lock(&(instance->connected_locker));
        keep_running=instance->client_connected>0;
        pthread_mutex_unlock(&(instance->connected_locker));

        pthread_mutex_lock(&(instance->rr_class_locker)); 
        connection_socket=instance->rr_classes[class_index];
        pthread_mutex_unlock(&(instance->rr_class_locker));
        class_index=(class_index+1)%num_connections;
        if(connection_socket==(-1)){
            continue;            
        }
        pthread_mutex_lock(&(instance->map_locker));
        bool empty=(instance->rr_map[connection_socket]).empty();
        pthread_mutex_unlock(&(instance->map_locker)); 
        if(empty){
            continue;
        }
        pthread_mutex_lock(&(instance->map_locker));
        int request_offset=instance->rr_map[connection_socket].front();
        instance->rr_map[connection_socket].pop();
        pthread_mutex_unlock(&(instance->map_locker));

        request r;
        r.connection_socket=connection_socket;
        r.offset=request_offset;
        instance->send_file_portion(r);
    }
    return nullptr;
}
void* Server::fifo_queue_handler(void * args){
    pthread_detach(pthread_self());
    Server* instance=static_cast<Server*>(args);
    bool keep_running=true;
    while(keep_running){
        pthread_mutex_lock(&(instance->connected_locker));
        keep_running=instance->client_connected>0;
        pthread_mutex_unlock(&(instance->connected_locker));
        //checks if there are any requests queued up
        pthread_mutex_lock(&(instance->queue_locker));
        bool empty=instance->fifo_queue.empty();
        pthread_mutex_unlock(&(instance->queue_locker));

        //if there are no requests, wait and check again.
        if(empty){
            continue; 
        }

        //if there are requests, entertain them.
        pthread_mutex_lock(&(instance->queue_locker));
        request req_to_serv=instance->fifo_queue.front();
        instance->fifo_queue.pop();
        pthread_mutex_unlock(&(instance->queue_locker));

        instance->send_file_portion(req_to_serv);   
    }
    return nullptr;
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
    serv_addr.sin_addr.s_addr =inet_addr(config.server_ip.c_str());
    serv_addr.sin_port = htons(config.server_port);

    // bind socket
    if (bind(listening_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(listening_socket);
        exit(1);
    }
    if (listen(listening_socket, config.n) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(listening_socket);        
        exit(1);
    }    
}
void Server::parse_offset(client_data* thread_cd,queue<int>&offsets){
    char* ptr=thread_cd->buffer;
    //reads the buffer till not encountering a null character
    while((*ptr)!=0){
        char ch=*ptr;
        if(ch=='\n'){
            
            //reaches end of the packet
            offsets.push(stoi(thread_cd->offset));
            thread_cd->offset="";
            ptr++;
            continue;
        }
        thread_cd->offset=thread_cd->offset+ch;
        ptr++;
    }
}
void Server::parse_request(client_data* thread_cd,queue<request>&requests){
    char* ptr=thread_cd->buffer;
    //reads the buffer till not encountering a null character
    while((*ptr)!=0){
        char ch=*ptr;
        if(ch=='\n'){

            //reaches end of the packet

            request r;
            r.offset=stoi(thread_cd->offset);
            r.connection_socket=thread_cd->connection_socket;
            requests.push(r);

            thread_cd->offset="";
            ptr++;
            continue;
        }
        thread_cd->offset=thread_cd->offset+ch;
        ptr++;
    }
}
void* Server::fifo_client_handler(void * args){
    
    pthread_detach(pthread_self());
    //has connected to the client after accepting connection 
    bool connected=true;

    ThreadArgs* thr_args=static_cast<ThreadArgs*>(args);
    client_data* thread_cd = static_cast<client_data*>(thr_args->cd);
    Server* instance = static_cast<Server*>(thr_args->instance);
    
    thread_cd->offset="";

    //entertains all requests while client chooses to remain connected
    while(connected){
        connected=instance->read_request(thread_cd);
        if(!connected){
            break;
        }
        pthread_mutex_lock(&(instance->queue_locker));
        instance->parse_request(thread_cd,(instance->fifo_queue));
        pthread_mutex_unlock(&(instance->queue_locker));
    }  
    //closes the connection with the client
    close(thread_cd->connection_socket);
    pthread_mutex_lock(&(instance->connected_locker));
    instance->client_connected--;
    pthread_mutex_unlock(&(instance->connected_locker));

    delete thr_args;
    return nullptr;
}
void* Server::rr_client_handler(void * args){
    pthread_detach(pthread_self());
    //has connected to the client after accepting connection 
    bool connected=true;

    ThreadArgs* thr_args=static_cast<ThreadArgs*>(args);
    client_data* thread_cd = static_cast<client_data*>(thr_args->cd);
    Server* instance = static_cast<Server*>(thr_args->instance);
    int i;

    //inserts the client into the map   
    pthread_mutex_lock(&(instance->map_locker));
    (instance->rr_map)[thread_cd->connection_socket]=queue<int>();
    pthread_mutex_unlock(&(instance->map_locker));

    pthread_mutex_lock(&(instance->rr_class_locker));
    for(i=0;i<(instance->rr_classes.size());i++){
        if(instance->rr_classes[i]==-1){
            instance->rr_classes[i]=thread_cd->connection_socket;
            break;
        }
    }
    pthread_mutex_unlock(&(instance->rr_class_locker));
    thread_cd->offset="";

    //entertains all requests while client chooses to remain connected
    while(connected){
        connected=instance->read_request(thread_cd);
        if(!connected){
            break;
        }
        queue <int>&offsets=(instance->rr_map)[thread_cd->connection_socket];
        pthread_mutex_lock(&(instance->map_locker));  
        instance->parse_offset(thread_cd,offsets); 
        pthread_mutex_unlock(&(instance->map_locker));
    }  

    //deletes the client from the map
    pthread_mutex_lock(&(instance->map_locker));    
    (instance->rr_map).erase(thread_cd->connection_socket);
    pthread_mutex_unlock(&(instance->map_locker)); 

    pthread_mutex_lock(&(instance->rr_class_locker));
    for(int i=0;i<(instance->rr_classes.size());i++){
        if((instance->rr_classes[i])==(thread_cd->connection_socket)){
            instance->rr_classes[i]=-1;
            break;
        }
    }
    pthread_mutex_unlock(&(instance->rr_class_locker));
    
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
    
    if(config.pol==fifo){
        //thread which entertains requests in the fifo order.
        pthread_t fifo_thread;
        if (pthread_create(&fifo_thread, nullptr, fifo_queue_handler, this) != 0) {
            cerr << "Error creating thread" << endl;
        }
    }
    else if(config.pol==round_robin){
        //thread which entertains requests in the round robin order.
        pthread_t rr_thread;
        if (pthread_create(&rr_thread, nullptr, rr_map_handler, this) != 0) {
            cerr << "Error creating thread" << endl;
        }        
    }
    //listens to connection requests forever
    int num_client_connected=config.n;
    while(num_client_connected>0){
        
        int connection_socket=accept_connection(); 
        num_client_connected--;
        char* buffer=new char[BUFFSIZE];
        client_data* cd = new client_data{buffer,"",connection_socket}; 
        ThreadArgs* args=new ThreadArgs{cd,this};
        //thread which actually connects to the client
        pthread_t connection_thread; 
        if(config.pol==fifo){
            if (pthread_create(&connection_thread, nullptr, fifo_client_handler, args) != 0) {
                cerr << "Error creating thread" << endl;
                close(connection_socket);
                continue;
            }
        }
        else if(config.pol==round_robin){
            if (pthread_create(&connection_thread, nullptr, rr_client_handler, args) != 0) {
                cerr << "Error creating thread" << endl;
                close(connection_socket);
                continue;
            }
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

int main(int argc, char* argv[]) {
    string policy_choice=argv[1];
    Server * server=new Server();
    
    server->load_config();
    
    server->set_policy(policy_choice);
    server->load_data();
    server->run();
    delete server;
}