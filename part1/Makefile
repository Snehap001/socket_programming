CC = g++
CFLAGS = -std=c++11
CLIENT = client
SERVER = server
EXPERIMENT=experiment
KILL_PROCESSES=kill_processes
build: $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(EXPERIMENT) $(EXPERIMENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(KILL_PROCESSES) $(KILL_PROCESSES).cpp -ljsoncpp
server:build
	./server 
client: ./client
run: build
	./kill_processes
	./server &
	./client 

plot: build
	./kill_processes
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) plot.png
