CC = g++
CFLAGS = -std=c++11
CLIENT = single_client
SERVER = server
MULT_CLIENTS=client
EXPERIMENT=experiment

build: $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(MULT_CLIENTS) $(MULT_CLIENTS).cpp -ljsoncpp


server:build
	./server
client: ./client
run: build
	./server &
	./client 

plot: build
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) $(MULT_CLIENTS) $(EXPERIMENT) client_time.csv plot.png
