line = "--------------------------------------------------------------------"

all:
	echo $(line); c++ main.cpp -std=c++11 -Wall -pedantic -g -o main -lGL -lglut -lGLU -lGLEW