OPTS=-O3 -g
INC=-I.. -I../..

all: FMM1d

FMM1d: FMM1d.cpp *.h
	g++ $< -o $@ $(OPTS) $(INC)

clean:
	rm -rf ./FMM1d
