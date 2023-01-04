CC = g++
C_FLAGS = -std=c++11

all: run

run: clean
	gnome-terminal -- ./server/server
	python3 client/client.py

clean: 
	rm -f inpipe outpipe