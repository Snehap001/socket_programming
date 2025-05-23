#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <random> 
#include <vector>
#include <chrono>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <set>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

enum protocol{slotted_aloha,beb,sensing_beb};
enum status{BUSY,IDLE};
struct client_config{
    string server_ip;
    int server_port,k,client_id;
    protocol prot;
    int T,n;
};
class Client{

    private:
    const int BUFFSIZE=1024;
    struct client_config config;
    struct sockaddr_in serv_addr;
    char* buffer;
    int communication_socket;
    string incomplete_packet;
    bool file_received;
    std::map<std::string, int> word_count;
    default_random_engine generator; 
    void request_contents(int offset);
    void wait_till_idle();
    void wait_for_slot(double prob,bool);
    void add_to_map(multiset<string> & words);
    void parse_packet(int & words_remaining,multiset<string> & words);
    status parse_status();
    void read_data();
    void backoff(int num_attempts);
    void download_file_sensing_beb();
    void download_file_beb();
    void download_file_slotted_aloha();

    public:
    Client(int id);
    void load_config();
    void open_connection();
    void set_protocol(string p);
    void add_time_entry(const string& filename, const vector<string>& new_row);
    void download_file();
    void dump_frequency();
    ~Client();
};
Client::Client(int id){
    buffer=new char[BUFFSIZE];
    config.client_id=id;
}
Client::~Client(){
    delete buffer;
}
void Client::set_protocol(string p){
    if(p=="aloha"){
        config.prot=slotted_aloha;
    }
    else if(p=="beb"){
        config.prot=beb;
    }
    else if(p=="cscd"){
        config.prot=sensing_beb;
    }
}
void Client::load_config() {

    //loads the configuration of client for communication
    ifstream config_file("config.json", std::ifstream::binary);
    json configuration;
    config_file >> configuration;
    config.server_ip = configuration["server_ip"].get<string>();
    config.server_port = configuration["server_port"].get<int>();
    config.k = configuration["k"].get<int>();
    config.T=configuration["T"].get<double>();
    config.n=configuration["num_clients"].get<int>();
    config_file.close();

}
void Client::open_connection() {
    //creates the socket for the data connection
    if ((communication_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
    }

    //creates the server adddress
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(config.server_port);
    if (inet_pton(AF_INET, config.server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        close(communication_socket);
    }
    //does the three-way handshake
    bool connection_established=false;
    while(!connection_established){
        if (connect(communication_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            connection_established=false;
        }
        else{
            connection_established=true;
        }
        sleep(1);
    }


}
void Client::request_contents(int offset){

    //sends the request containing the offset
    string request = (to_string(offset)+"\n");
    send(communication_socket, request.c_str(), request.size(), 0);
    incomplete_packet="";

}
void Client::read_data(){

    //clears the buffer
    memset(buffer, 0, BUFFSIZE);

    //reads the data from the receive queue into the buffer  
    int bytes_received = recv(communication_socket, buffer, BUFFSIZE-1,0);
    if (bytes_received < 0) {
        std::cerr << "Read error client" << std::endl;
        close(communication_socket);
        throw("error");
    }

}
status Client::parse_status(){
    char* ptr=buffer;
    string stat="";
    //reads the buffer till not encountering a null character
    while((*ptr)!=0){
        char ch=*ptr;
        if(ch=='\n'){
            if(stat=="IDLE"){
                return IDLE;
            }
            else if (stat=="BUSY"){
                return BUSY;
            }
            else{
                //throw error
                return BUSY;
            }
        }
        stat=stat+ch;
        ptr++;
    }
    return BUSY;
}
void Client::parse_packet(int & words_remaining,multiset<string> & words){
    string word=incomplete_packet;
    char* ptr=buffer;
    char ch;
    //reads the buffer till encountering null character
    while((*ptr)!=0){
        ch=*ptr;
        //on reaching end of word, adds it to map
        if(ch==','){
            words_remaining--;
            words.insert(word);
            word="";
            ptr++;
            continue;
        }
        //on reaching end of packet, checks if it is invalid/eof
        if(ch=='\n'){
            if(word=="HUH!"){
                words_remaining=-2;
                return;
            }
            if((word=="$$")||(word=="EOF")){
                file_received=true;
                words_remaining=-1;
                return;
            }
            words_remaining--;
            words.insert(word);
            word="";
            ptr++;
            continue;         
        }
        word=word+ch;
        ptr++;
    }
    incomplete_packet=word;
}
void Client::add_to_map(multiset<string> & words){
    for (auto & word: words){
        auto it = word_count.find(word);
        if (it != word_count.end()) {
            word_count[word]++;
        } 
        else {
            word_count.insert({word,1});
        }
    }
}
void Client::download_file(){
    switch(config.prot){
        case slotted_aloha:
            download_file_slotted_aloha();
            break;
        case beb:
            download_file_beb();
            break;
        case sensing_beb:
            download_file_sensing_beb();
            break;
    }
}
void Client::wait_till_idle(){
    status s=BUSY;
    unsigned int waiting_period=((config.T)*1000);

    while(s==BUSY){
        
        send(communication_socket, "BUSY?\n", 6, 0); 
        read_data();   
        s=parse_status();
      
        if(s==BUSY){
            usleep(waiting_period);
        }
    }
}
void Client::wait_for_slot(double prob,bool fresh_request){
    uniform_real_distribution<double> distribution (0.0,1.0); 
    generator.seed(time(0));
    while(true){
        int now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
        if((now%config.T)==0){
            if(fresh_request){
                return;
            }
            else{
                double toss=distribution(generator);
                if(toss<=prob){
                    return;
                }
            }
        }
    }
}
void Client::download_file_sensing_beb() {
   //sets up the connection
    file_received=false;
    int offset=0,words_remaining=0,num_attempts=0;
    //sends requests till the entire file is not received
    while(!file_received){
        wait_till_idle();
        request_contents(offset);
        words_remaining=config.k;
        multiset <string> words;
        //keeps reading till the requested data has not been read
        while(words_remaining>0){
            read_data();
            parse_packet(words_remaining,words);
        }
        if(words_remaining>=(-1)){
            add_to_map(words);
            num_attempts=0;
            offset+=config.k;
            continue;
        }
        num_attempts++;
        backoff(num_attempts);
    }
    //tells the server to close the conversation
    close(communication_socket);
}
void Client::download_file_beb() {
    //sets up the connection
    
    file_received=false;
    int offset=0;
    //sends requests till the entire file is not received
    int words_remaining;
    int num_attempts=0;
    while(!file_received){
        request_contents(offset);
        words_remaining=config.k;
        multiset <string> words;
        //keeps reading till the requested data has not been read
        while(words_remaining>0){
            read_data();
            parse_packet(words_remaining,words);
        }
        if(words_remaining>=(-1)){
            add_to_map(words);
            num_attempts=0;
            offset+=config.k;
            continue;
        }
        num_attempts++;
        backoff(num_attempts);
    }
    //tells the server to close the conversation
    close(communication_socket);
}
void Client::download_file_slotted_aloha() {
    //sets up the connection 
    file_received=false;
    int offset=0;
    //sends requests till the entire file is not received
    int words_remaining;
    double prob=(1/((double)(config.n)));
    time_t slot;
    bool fresh_request=true;
    while(!file_received){
        wait_for_slot(prob,fresh_request);
        request_contents(offset);
        words_remaining=config.k;
        multiset <string> words;
        //keeps reading till the requested data has not been read
        while(words_remaining>0){
            read_data();
            parse_packet(words_remaining,words);
        }
        if(words_remaining>=(-1)){
            add_to_map(words);
            offset+=config.k;
            fresh_request=true;
        }
        else{
            fresh_request=false;
        }
    }
    //tells the server to close the conversation
    close(communication_socket);
}
void Client::add_time_entry(const string& filename, const vector<string>& new_row) {
    ofstream file(filename, ios::app);
    if (!file.is_open()) {
        cerr << "Could not open the file!" << std::endl;
        return;
    }
    for (size_t i = 0; i < new_row.size(); ++i) {
        file << new_row[i];
        
    }
    file << "\n"; 
    file.close();
}
void Client::backoff(int num_attempts){
    int N=pow(2,num_attempts)-1;
    srand(static_cast<unsigned int>(time(0)));
    double random_i=(rand())%N;
    unsigned int waiting_period=((random_i*config.T)*1000);
    usleep(waiting_period);
}
void Client::dump_frequency(){
    //writes the frequencies to file
    std::ofstream outFile("client_"+to_string(config.client_id)+".txt");
    if (!outFile) {
        std::cerr << "Error opening file for writing!" << endl;
    }
    for (const auto& pair : word_count) {
        outFile << pair.first << ", " << pair.second << endl;
    }
    
    outFile.close();
}
int main(int argc, char* argv[]) {
    cout<<"client called"<<endl;
    string str(argv[1]);
    int id=0;
    for(char ch:str){
        id=(id*10)+((int)ch)-48;
    }
    Client *client=new Client(id);
    string prot=argv[2];

    client->load_config();
    client->set_protocol(prot);
    client->open_connection();
    auto start = chrono::high_resolution_clock::now();
    client->download_file();
    client->dump_frequency();
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    if(argc==4){ 
        
        vector<string>entry={to_string(duration.count())};
        client->add_time_entry("client_time.csv",entry);
        
    
    }
    delete client;
    return 0;
}