all:
	g++ -O2 -g main.cpp ../lib/simlib.so -o main -std=gnu++11

run: clean all
	./main

clean:
	rm ./main