SRC_FILES=ball1d.cpp
OPTS=-O3 -g
INC=-I ../ -I ../../
BUILD_OPTS = -c -std=c++11 $(OPTS) $(INC)
CHARM_OPTS =
TEST_OPTS = ++local
CHARMC = $(CHARM_HOME)/bin/charmc $(CHARM_OPTS)
OBJS=ball1d.o

all: ball1d

ball1d: compile
	$(CHARMC) -language charm++ -o $@ $(OBJS) $(LIBS)

compile: interface $(SRC_FILES)
	$(CHARMC) $(BUILD_OPTS) $(SRC_FILES) $(LIBS)

interface: ball1d.ci
	$(CHARMC) ball1d.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o ball1d charmrun

cleanp:
	rm -f *.sts *.gz *.projrc *.topo *.out

cleanpl:
	rm -f *.sts *.gz *.projrc *.topo *.out *.log
