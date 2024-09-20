CC = g++
CFLAGS = -std=c++11
CLIENT = single_client
SERVER = server
MULT_CLIENTS=client
EXPERIMENT=experiment
KILL_PROCESSES=kill_processes
build: $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp 
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(MULT_CLIENTS) $(MULT_CLIENTS).cpp 
	$(CC) $(CFLAGS) -o $(EXPERIMENT) $(EXPERIMENT).cpp 
	$(CC) $(CFLAGS) -o $(KILL_PROCESSES) $(KILL_PROCESSES).cpp 




server-aloha:build
	./kill_processes
	./server
server-beb:build
	./kill_processes
	./server 
server-cscd:build
	./kill_processes
	./server

client-aloha: ./client aloha
client-beb:./client beb
client-cscd:./client cscd

run-aloha: build
		./kill_processes
		./server &
		./client aloha
run-beb: build
		./kill_processes
		./server &
		./client beb
run-cscd:build
		./kill_processes
		./server &
		./client cscd		
run: build
	./kill_processes
	./server &
	./client all

plot: build
	./kill_processes
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) $(MULT_CLIENTS) $(EXPERIMENT) client_time.csv plot.png
