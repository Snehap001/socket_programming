CC = g++
CFLAGS = -std=c++11
CLIENT = client
SERVER = server
MULT_CLIENTS=mult_clients

build: $(CLIENT).cpp $(SERVER).cpp $(MULT_CLIENTS).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(MULT_CLIENTS) $(MULT_CLIENTS).cpp -ljsoncpp

run: build
	./server &
	./client 

plot: build
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) $(MULT_CLIENTS) plot.png
