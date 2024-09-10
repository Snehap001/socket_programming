CC = g++
CFLAGS = -std=c++11
CLIENT = client
SERVER = server
EXPERIMENT=experiment

build: $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(EXPERIMENT) $(EXPERIMENT).cpp -ljsoncpp
make server:./server &
make client: ./client
run: build
	./server &
	./client 

plot: build
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) plot.png
