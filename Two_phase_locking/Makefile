CC=g++
CFLAGS=-g --std=c++11

run: hw2.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f %.o run
