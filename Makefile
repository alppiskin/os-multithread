all: link

link: compile
	g++ main.o -o proj4 -lpthread -O3

compile:
	g++ -c main.cpp -O3	

clean:
	rm *.o
	rm proj4
