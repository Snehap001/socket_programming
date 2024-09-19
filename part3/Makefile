CC = g++
CFLAGS = -std=c++11
CLIENT = single_client
SERVER = server
MULT_CLIENTS=client
EXPERIMENT=experiment
KILL_PROCESSES=kill_processes
build: $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(MULT_CLIENTS) $(MULT_CLIENTS).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(EXPERIMENT) $(EXPERIMENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(KILL_PROCESSES) $(KILL_PROCESSES).cpp -ljsoncpp




server:build
	./server
client: ./client
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
