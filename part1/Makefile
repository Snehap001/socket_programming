CC = g++
CFLAGS = -std=c++11
CLIENT = client
SERVER = server

build: $(CLIENT).cpp $(SERVER).cpp
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp

run0: build
	./server &
run: run0
	./client     

plot: run
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) plot.png
