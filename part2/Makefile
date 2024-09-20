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

server:build
	./kill_processes
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
	rm -f $(CLIENT) $(SERVER) $(MULT_CLIENTS) $(EXPERIMENT) client_time.csv plot.png
