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

int PORT;
int CHUNK_SIZE;
int K;

// Helper function to split words from a string by a comma
std::vector<std::string> split_words(const std::string &data) {
    std::vector<std::string> words;
    std::istringstream ss(data);
    std::string word;
    while (getline(ss, word, ',')) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    return words;
}

// Handle the client request
void handle_client(int client_socket, char *file_content, size_t file_size) {
    char buffer[1024] = {0};  // Initialize buffer
    std::vector<std::string> words = split_words(file_content);
    int total_words = words.size();

    // Receive the offset from the client
    int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';  // Null-terminate the string
    int offset = std::stoi(buffer);

    // If offset is out of range, send an error message and close
    if (offset >= total_words) {
        const char *end_message = "ERROR: Offset out of range\n";
        send(client_socket, end_message, strlen(end_message), 0);
        close(client_socket);
        return;
    }

    // Prepare and send chunks of words
    int index = offset;
    while (index < offset + K && index < total_words) {
        std::string response;
        int chunk_end = std::min(index + CHUNK_SIZE, total_words);
        
        for (int i = index; i < chunk_end; ++i) {
            response += words[i] + ",";
        }
        
        // Remove trailing comma
        if (!response.empty() && response.back() == ',') {
            response.pop_back();
        }

        // Check if we need to add the EOF marker
        if (chunk_end == total_words) {
            response += ",EOF\n";
        }
        else{
            response += ",\n";  // End of chunk
        }
        
        send(client_socket, response.c_str(), response.size(), 0);

        index = chunk_end;  // Move to the next chunk
    }

    close(client_socket);
}

// Load server configuration from JSON
void load_config() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value config;
    config_file >> config;

    PORT = config["server_port"].asInt();
    CHUNK_SIZE = config["p"].asInt();
    K = config["k"].asInt();
}

int main() {
    load_config();
    
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
  
    int opt=1;
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen on the socket
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    const char *file_path = "word.txt";
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("file open failed");
        exit(EXIT_FAILURE);
    }

    // Memory map the file content
    size_t file_size = lseek(fd, 0, SEEK_END);
    char *file_content = static_cast<char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_content == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Accept and handle incoming connections
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        handle_client(client_socket, file_content, file_size);
    }

    // Clean up resources
    munmap(file_content, file_size);
    close(fd);
    close(server_fd);
    return 0;
}
