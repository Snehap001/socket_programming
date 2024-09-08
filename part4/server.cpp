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
#include <queue>
#include <pthread.h>
#include <jsoncpp/json/json.h>  // Include JSON library
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <arpa/inet.h>
using namespace std;

struct server_config {
    int server_port, k, p, n;
    const char* fname;
};


struct client_data {
    char* buffer;
    string offset;
    int connection_socket;
    ~client_data() { if (buffer) { delete buffer; } }
};

class Server {
private:
    const int BUFFSIZE = 1024;
    struct server_config config;
    struct sockaddr_in serv_addr;
    int listening_socket;
    vector<string> file;

    queue<client_data*> fifo_queue;
    unordered_map<string, queue<client_data*>> client_request_map;
    vector<string> client_queue;  // To maintain round-robin order
    size_t current_rr_index=0;
    mutex queue_mutex;
    condition_variable queue_cv;
    bool server_running;

    void send_file_portion(client_data* thread_cd);
    bool parse_request(client_data* thread_cd);
    void open_listening_socket();
    bool read_request(client_data* thread_cd);
    int accept_connection();
    static void* connection_handler(void* args);
    static void* fifo_scheduler(void* args);
    static void* round_robin_scheduler(void* args);
    void add_client_request(int connection_socket);
public:
    Server();
    void load_config();
    void load_data();
    void run(bool isFifo);
    ~Server();
};

Server::Server() : server_running(true) {
}

Server::~Server() {
    server_running = false;
    close(listening_socket);
}
struct handler_args {
    Server* server;
    bool isFifo;
};

void Server::load_config() {
    // Loads the server configuration from the config file
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_port = configuration["server_port"].asInt();
    config.p = configuration["p"].asInt();
    config.k = configuration["k"].asInt();
    config.n = configuration["n"].asInt();
    config.fname = (configuration["filename"]).asString().c_str();

}

void Server::load_data() {
    // Loads the data from the file
    ifstream f(config.fname);
    string word;
    while (getline(f, word, ',')) {
        if (!word.empty()) {
            file.push_back(word);
        }
    }
    file.push_back("EOF");
}

void Server::send_file_portion(client_data* thread_cd) {
    int index = stoi(thread_cd->offset);
    int max_index = file.size() - 2;
    string packet = "";
    if (index >= max_index) {
        packet = "$$\n";
        send(thread_cd->connection_socket, packet.c_str(), packet.size(), 0);
        return;
    }
    int num_words = config.k;
    int packet_size = config.p;
    int last_index = index + num_words - 1;
    if (last_index >= max_index) {
        last_index = file.size() - 1;
    }
    while (index <= last_index) {
        packet = "";
        int this_packet_size = 0;
        while (this_packet_size < packet_size) {
            if (index > last_index) {
                break;
            }
            packet = packet + file[index] + ",";
            this_packet_size++;
            index++;
        }
        packet.pop_back();
        packet = packet + "\n";
        send(thread_cd->connection_socket, packet.c_str(), packet.size(), 0);
    }
}

bool Server::read_request(client_data* thread_cd) {
    // Clears the buffer
    memset(thread_cd->buffer, 0, BUFFSIZE);

    // Reads the data from the receive queue into the buffer  
    ssize_t bytes_received = recv(thread_cd->connection_socket, thread_cd->buffer, BUFFSIZE - 1, 0);
    // If connection has been closed, return false

    if (bytes_received == 0) {
        return false;
    }
    if (bytes_received < 0) {
        std::cerr << "Read error server" << std::endl;
        close(thread_cd->connection_socket);
        exit(1);
    }
    return true;
}

void Server::open_listening_socket() {
    // Create socket
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(1);
    }

    int opt = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Creates the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(config.server_port);

    // Bind socket
    if (bind(listening_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(listening_socket);
        exit(1);
    }
}

bool Server::parse_request(client_data* thread_cd) {
    char* ptr = thread_cd->buffer;
    // Reads the buffer till not encountering a null character
    while ((*ptr) != 0) {
        char ch = *ptr;
        if (ch == '\n') {
            // Reaches end of the packet
            return false;
        }
        thread_cd->offset += ch;
        ptr++;
    }
    // Has not reached end of the packet
    return true;
}

int Server::accept_connection() {
    // Wait for a connection request
    if (listen(listening_socket, 10) < 0) {  // Allow a queue of 10 connections
        std::cerr << "Listen failed" << std::endl;
        close(listening_socket);
        exit(1);
    }

    // Accept request to connect
    struct sockaddr_in clnt_addr;
    socklen_t addrlen = sizeof(clnt_addr);
    int connection_socket = accept(listening_socket, (struct sockaddr*)&clnt_addr, &addrlen);
    return connection_socket;
}

void* Server::connection_handler(void* args) {
    // Unpack the arguments
    auto handler_args = static_cast<pair<Server*, bool>*>(args);
    Server* server = handler_args->first;
    bool isFifo = handler_args->second;
    delete handler_args;  // Clean up the argument struct after unpacking

    while (server->server_running) {  
        // Accept client connection
        int connection_socket = server->accept_connection();
        

        // Lock the queue and add the client data
        {
            lock_guard<mutex> lock(server->queue_mutex);
            if (isFifo) {
                char* buffer = new char[server->BUFFSIZE];
                client_data* cd = new client_data{ buffer, "", connection_socket };
                // Add to FIFO queue
                server->fifo_queue.push(cd);
                server->queue_cv.notify_one();
            } else {
                // Add to Round Robin queue
            
                server->add_client_request(connection_socket);
            }
        }
        // Notify the worker thread that a new client has been added
        
    }

    return nullptr;
}
void Server::add_client_request(int connection_socket) {
    // Get client IP and port to generate a unique ID
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(connection_socket, (struct sockaddr*)&addr, &addr_len);
    string client_id = inet_ntoa(addr.sin_addr) + to_string(ntohs(addr.sin_port));
    // Create a new client_data object for the request
    char* buffer = new char[BUFFSIZE];
    client_data* cd = new client_data{ buffer, "", connection_socket };
    // Lock and add the client’s request to the map and queue
    
    // lock_guard<mutex> lock(queue_mutex);
    client_request_map[client_id].push(cd);
    // If this is a new client, add them to the round-robin queue
    if (client_request_map[client_id].size() == 1) {
        client_queue.push_back(client_id);
    }
    
    queue_cv.notify_one();
   
    
}


void* Server::fifo_scheduler(void* args) {
    Server* server = static_cast<Server*>(args);

    while (server->server_running) {
        client_data* cd = nullptr;

        // Wait until there is a client request in the queue
        {
            unique_lock<mutex> lock(server->queue_mutex);
            server->queue_cv.wait(lock, [server]() { return !server->fifo_queue.empty(); });

            // Retrieve the next client from the queue
            cd = server->fifo_queue.front();
            server->fifo_queue.pop();
        }

        // Process the client request
        bool connected = true;
        while (connected) {
            bool request_remaining = true;
            cd->offset = "";
            // Reads the request packet entirely
            while (request_remaining) {
                connected = server->read_request(cd);
                if (!connected) {
                    break;
                }
                request_remaining = server->parse_request(cd);
            }
            if (!connected) {
                break;
            }
            // Sends the file from the requested offset
            server->send_file_portion(cd);
        }
        // Closes the connection with the client
        close(cd->connection_socket);
        delete cd;
    }

    return nullptr;
}
void* Server::round_robin_scheduler(void* args) {
    Server* server = static_cast<Server*>(args);

    while (server->server_running) {
        string client_id;
        client_data* cd = nullptr;

        // Wait until there is at least one client in the queue
        {
            unique_lock<mutex> lock(server->queue_mutex);
            server->queue_cv.wait(lock, [server]() { return !server->client_queue.empty(); });

            // Retrieve the next client in the round-robin queue
            client_id = server->client_queue[server->current_rr_index];
            cd = server->client_request_map[client_id].front();  // Get the next request from the client

            // Remove the processed request from the queue
            server->client_request_map[client_id].pop();

            
        }

        // Process the client request (similar to previous logic)
        bool connected = true;
        while (connected) {
            bool request_remaining = true;
            cd->offset = "";
            // Reads the request packet entirely
            while (request_remaining) {
                connected = server->read_request(cd);
                if (!connected) {
                    break;
                }
                request_remaining = server->parse_request(cd);
            }
            if (!connected) {
                break;
            }
            // Sends the file from the requested offset
            server->send_file_portion(cd);
        }

        // After processing, check if the client still has pending requests
        {
            lock_guard<mutex> lock(server->queue_mutex);
            if(server->client_request_map[client_id].size()==0){
                for( int i=0;i<server->client_queue.size();i++){
                    if(server->client_queue[i]==client_id){
                        server->client_queue.erase(server->client_queue.begin()+i);
                        break;
                    }
                }
            }
            if(server->client_queue.size()!=0){
                server->current_rr_index = (server->current_rr_index + 1) % server->client_queue.size();
            }
            else{
                server->current_rr_index = 0;
            }
            
            
        }

        // Close the connection with the client
        close(cd->connection_socket);
        delete cd;
    }

    return nullptr;
}
void Server::run(bool isFifo) {
    // Opens the server's socket for listening to connection requests
    open_listening_socket();
   
    // Create connection handler thread
    pthread_t connection_thread_id;
    handler_args* args = new handler_args{ this, isFifo };

    pthread_create(&connection_thread_id, nullptr, connection_handler, args);
 
    // Create FIFO scheduler thread
    if(isFifo){
        pthread_t scheduler_thread_id;
        pthread_create(&scheduler_thread_id, nullptr, fifo_scheduler, this);

        // Wait for both threads to finish 
        pthread_join(connection_thread_id, nullptr);
        pthread_join(scheduler_thread_id, nullptr);
    }
    else{
        pthread_t scheduler_thread_id;
        pthread_create(&scheduler_thread_id, nullptr, round_robin_scheduler, this);

        // Wait for both threads to finish 
        pthread_join(connection_thread_id, nullptr);
        pthread_join(scheduler_thread_id, nullptr);
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
