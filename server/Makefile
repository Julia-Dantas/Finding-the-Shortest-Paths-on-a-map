CC = g++
C_FLAGS = -std=c++11

all: server

server: clean
	$(CC) -c dijkstra.cpp digraph.cpp server.cpp 
	$(CC) $(C_FLAGS) -o server dijkstra.o digraph.o server.o

dijkstra.o: dijkstra.cpp dijkstra.h wdigraph.h digraph.h
	$(CC) -c dijkstra.cpp

server.o: server.cpp digraph.h wdigraph.h dijkstra.h
	$(CC) -c server.cpp

digraph.o: digraph.cpp digraph.h
	$(CC) -c digraph.cpp

clean: 
	rm -f server dijkstra.o digraph.o server.o
