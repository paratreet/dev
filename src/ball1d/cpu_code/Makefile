OPTS=-O3 -g
INC=-I.. -I../..

all: ball1d

ball1d: ball1d.cpp *.h
	g++ $< -o $@ $(OPTS) $(INC)

clean:
	rm -rf ./ball1d
