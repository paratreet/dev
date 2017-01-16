OPTS=-O3 -g -std=c++11 #-U__CHARMC__
INC=-I../ -I../../

all: barnes3d

barnes3d: barnes3d.cpp *.h
	g++ $< -o $@ $(OPTS) $(INC)

clean:
	rm -rf ./barnes3d
