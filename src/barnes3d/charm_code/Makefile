SRC_FILES=barnes3d.cpp
OPTS=-O3 -g
INC=-I ../ -I ../../
BUILD_OPTS = -c -std=c++11 $(OPTS) $(INC)
CHARM_OPTS =
TEST_OPTS = ++local
CHARMC = $(CHARM_HOME)/bin/charmc $(CHARM_OPTS)

TARGET = barnes3d
OBJS = $(TARGET).o

all: $(TARGET)

barnes3d: compile
	$(CHARMC) -language charm++ -o $@ $(OBJS) $(LIBS)

compile: interface $(SRC_FILES)
	$(CHARMC) $(BUILD_OPTS) $(SRC_FILES) $(LIBS)

interface: barnes3d.ci
	$(CHARMC) barnes3d.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o barnes3d charmrun

cleanp:
	rm -f *.sts *.gz *.projrc *.topo *.out

cleanpl:
	rm -f *.sts *.gz *.projrc *.topo *.out *.log
