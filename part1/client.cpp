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
#include <jsoncpp/json/json.h>  // Include JSON library
#include <unordered_map>
int PORT;
std::string SERVER_IP;
int K;
int P;
std::string FILENAME;
std::unordered_map<std::string,int>word_map;
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

void load_config() {
    std::ifstream config_file("config.json", std::ifstream::binary);
    Json::Value config;
    config_file >> config;

    SERVER_IP = config["server_ip"].asString();
    PORT = config["server_port"].asInt();
    K = config["k"].asInt();
    P = config["p"].asInt();
    FILENAME = config["filename"].asString();
}

double run_experiment() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    int offset = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    std::map<std::string, int> word_count;

    while (true) {
        std::string request = std::to_string(offset);
        send(sock, request.c_str(), request.size(), 0);

        int bytes_received = read(sock, buffer, sizeof(buffer));
        if (bytes_received < 0) {
            std::cerr << "Read error" << std::endl;
            break;
        }

        std::string response(buffer, bytes_received);
        if (response == "EOF") {
            break;
        }

        // Split the response into words and count them
        size_t start = 0, end = 0;
       
        while ((end = response.find(',', start)) != std::string::npos) {
            std::string word = response.substr(start, end - start);
            if (!word.empty()) {
                ++word_count[word];
            }
            start = end + 1;
        }
        // Handle the last word (if any) or words without commas
        if (start < response.size()) {
            std::string word = response.substr(start);
            if (!word.empty()) {
                ++word_count[word];
            }
        }
        std::istringstream iss(response);
        std::vector<std::string> words;
        std::string word;

        while (iss >> word) {
            words.push_back(word);
        }

        for (const auto& w : words) {
            word_map[w]++;
        }


        // Update offset (assuming offset increases by the number of words processed)
        offset += K;

        // offset += words.size();
    }
    for (auto it:word_map) {
        std::cout << it.first << ": " << it.second << std::endl;
    }
   

    close(sock);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    return duration.count();
}

int main() {
    load_config();
    std::vector<double> times;

    for (int i = 1; i <= 10; ++i) {
        P = i;
        double total_time = 0;
        for (int j = 0; j < 10; ++j) {
            double time_taken = run_experiment();
            total_time += time_taken;
        }
        double avg_time = total_time / 10.0;
        times.push_back(avg_time);
    }

    std::ofstream out("times.txt");
    for (const auto &time : times) {
        out << time << std::endl;
    }
    out.close();

    return 0;
}
