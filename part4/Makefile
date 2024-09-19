CC = g++
CFLAGS = -std=c++11
CLIENT = single_client
SERVER = server
MULT_CLIENTS=client
EXPERIMENT=experiment
COMBINE=combine_results
ROGUE_CLIENT=rogue_client
KILL_PROCESSES=kill_processes
build: $(CLIENT).cpp $(CLIENT).cpp $(SERVER).cpp 
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(MULT_CLIENTS) $(MULT_CLIENTS).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(EXPERIMENT) $(EXPERIMENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(ROGUE_CLIENT) $(ROGUE_CLIENT).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(KILL_PROCESSES) $(KILL_PROCESSES).cpp -ljsoncpp
	$(CC) $(CFLAGS) -o $(COMBINE) $(COMBINE).cpp 

run-fifo: build
	./kill_processes
	./server fifo &
	./client fifo
run-rr: build
	./kill_processes
	./server rr &
	./client rr
run: build 
	./kill_processes
	./server fifo &
	./client
	./kill_processes
	./server rr &
	./client
	./combine_results

fairness: build 
	./kill_processes
	./server fifo &
	./client fairness fifo
	
	



plot: build
	./kill_processes
	./experiment
	python3 plot.py

clean:
	rm -f $(CLIENT) $(SERVER) $(MULT_CLIENTS) $(EXPERIMENT) $(KILL_PROCESSES) $(COMBINE) client_time.csv plot.png
