CFLAGS := -std=c11 -g -Wall -Wextra -fsanitize=address -fsanitize=undefined
CXXFLAGS := -std=c++17 -g -Wall -Wextra -fsanitize=address -fsanitize=undefined

main: des.o main.o
	$(CXX) $(CXXFLAGS) des.o main.o -o main
des.o: des.c des.h
	$(CC) $(CFLAGS) -c des.c -o des.o
main.o: main.cc des.h
	$(CXX) $(CXXFLAGS) -c main.cc -o main.o

.PHONY: clean
clean:
	rm -f *.o main
