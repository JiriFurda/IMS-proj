all:
	g++ -O2 -g ./src/main.cpp ./lib/simlib.so -o ./src/main -std=gnu++11

run: all
	./src/main

clean:
	rm ./src/main
