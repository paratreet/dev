OPTS=-O
CHARM_PATH=../charm
CHARMC=$(CHARM_PATH)/bin/charmc $(OPTS)

OBJS = main.o

all:	cifiles main

main: $(OBJS)
	$(CHARMC) -language charm++ -module collidecharm -o main $(OBJS)

cifiles: main.ci
	$(CHARMC)  main.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o main charmrun

main.o: cifiles main.cpp
	$(CHARMC) -c main.cpp

test: all
	$(call run, ./main +p4 10 )

