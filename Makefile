SOURCES=$(wildcard *.c)
	
all: clean	
	gcc -std=c99 -Wall $(SOURCES) -o main	

clean: 
	rm -f ./main
