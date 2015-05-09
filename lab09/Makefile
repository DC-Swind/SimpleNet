GITFLAGS = -m "Makefile auto commit" --no-verify --allow-empty

all: son/son sip/sip 
common/pkt.o: common/pkt.c common/pkt.h common/constants.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
topology/topology.o: topology/topology.c 
	gcc -Wall -pedantic -std=c99 -g -c topology/topology.c -o topology/topology.o
son/neighbortable.o: son/neighbortable.c
	gcc -Wall -pedantic -std=c99 -g -c son/neighbortable.c -o son/neighbortable.o
son/son: topology/topology.o common/pkt.o son/neighbortable.o son/son.c 
	gcc -Wall -pedantic -std=c99 -g -pthread son/son.c topology/topology.o common/pkt.o son/neighbortable.o -o son/son
sip/sip: common/pkt.o topology/topology.o sip/sip.c 
	gcc -Wall -pedantic -std=c99 -g -pthread common/pkt.o topology/topology.o sip/sip.c -o sip/sip 

clean:
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf son/*.o
	rm -rf son/son
	rm -rf sip/*.o
	rm -rf sip/sip 

# 定义伪目标
.PHONY: git pull push
pull:
	-@git pull origin master
push:
	-@git push origin master
git:	
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf son/*.o
	rm -rf son/son
	rm -rf sip/*.o
	rm -rf sip/sip 
	-@git add . --ignore-errors &> /dev/null # KEEP IT
	-@git commit $(GITFLAGS) # KEEP IT

playson:
	./son/son

playsip:
	./sip/sip



