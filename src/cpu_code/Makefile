OPTS=-O3 -g
INC=-I..

all: barnes1d

barnes1d: barnes1d.cpp *.h
	g++ $< -o $@ $(OPTS) $(INC)

clean:
	rm -rf ./barnes1d
