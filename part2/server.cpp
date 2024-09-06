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
#include <jsoncpp/json/json.h>  
#include <pthread.h>
#include <unordered_map>

using namespace std;

struct server_config {
    int server_port, k, p, n;
    unordered_map<int, string> offsets;
};

struct ThreadArgs {
    int client_socket;
    class Server* server_instance;
};

class Server {
private:
    const int BUFFSIZE = 1024;
    int MAX_CONNECTIONS = 5;
    struct server_config config;
    struct sockaddr_in serv_addr;

    char* buffer;
    int server_socket;
    vector<string> file;
    vector<pthread_t> threads;

    static void* manage_connection_helper(void* args);  

    void send_file(int client_socket);
    bool parse_request(int client_socket);
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

Server::Server() {
    buffer = new char[BUFFSIZE];
    MAX_CONNECTIONS = config.n;
}

Server::~Server() {
    delete[] buffer;
}

void Server::load_config() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value configuration;
    config_file >> configuration;
    config.server_port = configuration["server_port"].asInt();
    config.p = configuration["p"].asInt();
    config.k = configuration["k"].asInt();
    config.n = configuration["n"].asInt();
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

void Server::send_file(int client_socket) {
    int index = stoi(config.offsets[client_socket]);
    int max_index = file.size() - 2;
    string packet = "";

    if (index >= max_index) {
        packet = "$$\n";
        send(client_socket, packet.c_str(), packet.size(), 0);
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
        send(client_socket, packet.c_str(), packet.size(), 0);
    }
}

void Server::read_request(int client_socket) {
    memset(buffer, 0, BUFFSIZE);
    ssize_t bytes_received = recv(client_socket, buffer, BUFFSIZE - 1, 0);
    if (bytes_received < 0) {
        std::cerr << "Read error server" << std::endl;
        close(server_socket);
        close(client_socket);
        exit(1);
    }
}

void Server::manage_connection(int client_socket) {
    bool request_remaining = true;
    while (request_remaining) {
        read_request(client_socket);
        request_remaining = parse_request(client_socket);
    }
    send_file(client_socket);
    close(client_socket);
}

int Server::accept_connection() {
    struct sockaddr_in clnt_addr;
    if (listen(server_socket, MAX_CONNECTIONS + 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_socket);
        exit(1);
    }

    socklen_t addrlen = sizeof(clnt_addr);
    int client_socket = accept(server_socket, (struct sockaddr*)&clnt_addr, &addrlen);
    config.offsets[client_socket] = "";
    return client_socket;
}

void Server::open_socket() {
    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(config.server_port);

    if (bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }
}

void* Server::manage_connection_helper(void* args) {
    ThreadArgs* thread_args = static_cast<ThreadArgs*>(args);
    thread_args->server_instance->manage_connection(thread_args->client_socket);
    delete thread_args;  
    return nullptr;
}

void Server::create_threads() {
    while (threads.size() < config.n) {
        int client_socket = accept_connection();
        
        pthread_t thread;
        ThreadArgs* args = new ThreadArgs{client_socket, this};  // Passing both client socket and server instance

        if (pthread_create(&thread, nullptr, manage_connection_helper, args) != 0) {
            cerr << "Error creating thread" << endl;
            close(client_socket);
        } else {
            threads.push_back(thread);
        }
    }

    for (auto& th : threads) {
        pthread_join(th, nullptr);
    }

    close(server_socket);
}

void Server::connect() {
    open_socket();
    create_threads();
}

bool Server::parse_request(int client_socket) {
    for (int i = 0; i < BUFFSIZE - 1; i++) {
        char ch = buffer[i];
        if (ch == '\n') {
            return false;
        }

        config.offsets[client_socket] = config.offsets[client_socket] + ch;
    }
    return true;
}

int main() {
    Server* server = new Server();
    server->load_config();
    server->load_data("word.txt");
    server->connect();
    delete server;
}
