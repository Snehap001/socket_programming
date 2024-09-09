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
#include <jsoncpp/json/json.h>  // Include JSON library
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
using namespace std;
struct server_config{
    int server_port,k,p;
    const char* fname;
};
struct client_data{
    char* buffer;
    string offset;
    int connection_socket;
    ~client_data(){if(buffer){delete [] buffer;}}
};

class Server{
    private:
    const int BUFFSIZE=1024;   
    const int MAX_CONNECTIONS=1;
    struct server_config config;
    struct sockaddr_in serv_addr;
    int listening_socket;
    vector<string> file;
    condition_variable queue_cv;
    queue<client_data*>fifo_queue;
    mutex queue_mutex;
    unordered_map<int,queue<client_data*>>rr_map;
    vector<int>rr_queue;
    int current_rr_index=0;
    void send_file_portion(client_data* thread_cd);
    bool parse_request(client_data* thread_cd);
    static void* fifo_scheduler(void * cd);
    static void* rr_scheduler(void * cd);
    int accept_connection();
    void open_listening_socket();
    bool read_request(client_data* thread_cd);
    static void* fifo_handler(void *args);
    static void* rr_handler(void *args);
    public:
    Server();
    void load_config();
    void load_data();
    void run(bool isFifo);
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
void Server::send_file_portion(client_data* thread_cd){
    int index=stoi(thread_cd->offset);
    cout<<"offset "<<index<<endl;
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
        cout<<packet<<endl;
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
    cout<<"accept"<<endl;

    //accept request to connect
    struct sockaddr_in clnt_addr;
    socklen_t addrlen=sizeof(clnt_addr);
    int connection_socket = accept(listening_socket, (struct sockaddr *)&clnt_addr, &addrlen);
   
    cout<<connection_socket<<endl;
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
void* Server::fifo_scheduler(void * args){
    cout<<"fifo_scheduler"<<endl;
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
        {
            lock_guard<mutex> lock(instance->queue_mutex);
            char* buffer=new char[instance->BUFFSIZE];
            cout<<thread_cd->offset<<endl;
            client_data* cd=new client_data{buffer,thread_cd->offset,thread_cd->connection_socket};
            instance->fifo_queue.push(cd);
        }
            instance->queue_cv.notify_one();
        
        
    }  
    //closes the connection with the client
    close(thread_cd->connection_socket);
    delete thr_args;
    return nullptr;
}
void* Server::rr_scheduler(void * args){
    
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
        {
            lock_guard<mutex> lock(instance->queue_mutex);
            char* buffer=new char[instance->BUFFSIZE];
            client_data* cd=new client_data{buffer,thread_cd->offset,thread_cd->connection_socket};
            instance->rr_map[thread_cd->connection_socket].push(cd);
            if(instance->rr_map[thread_cd->connection_socket].size()==1){
                instance->rr_queue.push_back(thread_cd->connection_socket);
            }
        }
        instance->queue_cv.notify_one();
        
        
        
    }  
    //closes the connection with the client
    close(thread_cd->connection_socket);
    delete thr_args;
    return nullptr;
}
void* Server::fifo_handler(void *args){
    pthread_detach(pthread_self());
    Server* server = static_cast<Server*>(args);
    client_data *cd=nullptr;
    while(true){
        
       {    
            unique_lock<mutex> lock(server->queue_mutex);
       

            server->queue_cv.wait(lock, [server]() { return !server->fifo_queue.empty(); });
       
            cd=server->fifo_queue.front();
            server->fifo_queue.pop();
       }
        if(cd!=nullptr){
            server->send_file_portion(cd);
            delete cd;
        }
            
     
    }
    return nullptr;
}
void* Server::rr_handler(void *args){
    pthread_detach(pthread_self());
    Server* server = static_cast<Server*>(args);
    int socket;;
    client_data *cd=nullptr;
    while(true){
        
           { 
            unique_lock<mutex> lock(server->queue_mutex);
            server->queue_cv.wait(lock, [server]() { return !server->rr_queue.empty(); });
        
            int socket=server->rr_queue[server->current_rr_index];
            cd=server->rr_map[socket].front();
            server->rr_map[socket].pop();
            
            
            
            if(server->rr_map[socket].empty()){
                for(int i=0;i<server->rr_queue.size();i++){
                    if(server->rr_queue[i]==socket){
                        server->rr_queue.erase(server->rr_queue.begin()+i);
                        break;
                    }
                }
                server->rr_map.erase(socket);
            }
            else{
                if(server->rr_queue.size()!=0)
                    server->current_rr_index=(server->current_rr_index+1)%server->rr_queue.size();
                else{
                    server->current_rr_index=0;
                }

            }}
            if(cd!=nullptr){
                server->send_file_portion(cd);
                delete cd;
            }
            
        
    }
    return nullptr;
}

void Server::run(bool isFifo){
    //opens the server's socket for listening to connection requests
    open_listening_socket();
    //listens to connection requests forever
    if(isFifo){
        pthread_t fifo_thread; 
        if (pthread_create(&fifo_thread, nullptr, fifo_handler,this) != 0) {
                cerr << "Error creating thread" << endl;
                
        }
    }
    else{
        pthread_t rr_thread; 
        
        if (pthread_create(&rr_thread, nullptr, rr_handler,this) != 0) {
                cerr << "Error creating thread" << endl;
                
        }
    }
 
    while(true){
        int connection_socket=accept_connection(); 
        cout<<"conn"<<connection_socket<<endl;
        char* buffer=new char[BUFFSIZE];
        client_data* cd = new client_data{buffer,"",connection_socket}; 
        ThreadArgs* args=new ThreadArgs{cd,this};
        pthread_t thread; 
        if(isFifo){
            if (pthread_create(&thread, nullptr, fifo_scheduler, args) != 0) {
                cerr << "Error creating thread" << endl;
                close(connection_socket);
                continue;
            }
        }
        else{
            if (pthread_create(&thread, nullptr, rr_scheduler, args) != 0) {
                cerr << "Error creating thread" << endl;
                close(connection_socket);
                continue;
            }
        }
        
    }
}
int main(int argc, char* argv[]) {
    string fifo=argv[1];
   
    bool isFifo=false;
    if( fifo=="fifo"){
        isFifo=true;
    }
    Server* server = new Server();
    server->load_config();
    server->load_data();
 
    server->run(isFifo);
    delete server;
}