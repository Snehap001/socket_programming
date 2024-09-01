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

std::vector<std::string> split_words(const std::string &data) {
    std::vector<std::string> words;
    std::istringstream ss(data);
    std::string word;
    while (getline(ss, word, ',')) {
        words.push_back(word);
    }
    return words;
}

void handle_client(int client_socket, char *file_content, size_t file_size) {
    int offset = 0;
    int bytes_received;
    char buffer[1024] = {0};
    std::vector<std::string> words = split_words(file_content);
    int total_words = words.size();

    while ((bytes_received = read(client_socket, buffer, sizeof(buffer))) > 0) {
        std::string request(buffer);
        std::cout << "Request received: " << request << std::endl;

        offset = atoi(request.c_str());

        if (offset >= total_words) {
            const char *end_message = "EOF";
            send(client_socket, end_message, strlen(end_message), 0);
            break;
        }

        std::string response;
        for (int i = 0; i < CHUNK_SIZE && (offset + i) < total_words; ++i) {
            response += words[offset + i] + ",";
        }

        send(client_socket, response.c_str(), response.size(), 0);
    }

    close(client_socket);
}

void load_config() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value config;
    config_file >> config;

    PORT = config["server_port"].asInt();
    CHUNK_SIZE = config["p"].asInt();
}

int main() {
    load_config();

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    const char *file_path = "words.txt";
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("file open failed");
        exit(EXIT_FAILURE);
    }

    size_t file_size = lseek(fd, 0, SEEK_END);
    char *file_content = static_cast<char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_content == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        handle_client(client_socket, file_content, file_size);
    }

    munmap(file_content, file_size);
    close(fd);
    close(server_fd);
    return 0;
}
