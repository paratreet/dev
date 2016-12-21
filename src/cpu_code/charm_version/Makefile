SRC_FILES=barnes1d.cpp
BUILD_OPTS = -c -std=c++11 -g
CHARM_OPTS =
TEST_OPTS = ++local
CHARMC = $(CHARM_HOME)/bin/charmc $(CHARM_OPTS)
OBJS=barnes1d.o

all: barnes1d

barnes1d: compile
	$(CHARMC) -language charm++ -o $@ $(OBJS) $(LIBS)

compile: interface $(SRC_FILES)
	$(CHARMC) $(BUILD_OPTS) $(SRC_FILES) $(LIBS)

interface: barnes1d.ci
	$(CHARMC) barnes1d.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o barnes1d charmrun

cleanp:
	rm -f *.sts *.gz *.projrc *.topo *.out

cleanpl:
	rm -f *.sts *.gz *.projrc *.topo *.out *.log
