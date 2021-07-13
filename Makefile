.PHONY:clean
cc = g++
FLAGS = -std=c++11	-o3
delete: main.cpp
	$(cc) $(FLAGS) main.cpp -o delete

clean:
	-rm delete