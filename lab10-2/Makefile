GITFLAGS = -m "Makefile auto commit" --no-verify --allow-empty

all: son/son sip/sip client/client server/server  

common/pkt.o: common/pkt.c common/pkt.h common/constants.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
topology/topology.o: topology/topology.c 
	gcc -Wall -pedantic -std=c99 -g -c topology/topology.c -o topology/topology.o
son/neighbortable.o: son/neighbortable.c
	gcc -Wall -pedantic -std=c99 -g -c son/neighbortable.c -o son/neighbortable.o
son/son: topology/topology.o common/pkt.o son/neighbortable.o son/son.c 
	gcc -Wall -pedantic -std=c99 -g -pthread son/son.c topology/topology.o common/pkt.o son/neighbortable.o -o son/son
sip/nbrcosttable.o: sip/nbrcosttable.c
	gcc -Wall -pedantic -std=c99 -g -c sip/nbrcosttable.c -o sip/nbrcosttable.o
sip/dvtable.o: sip/dvtable.c
	gcc -Wall -pedantic -std=c99 -g -c sip/dvtable.c -o sip/dvtable.o
sip/routingtable.o: sip/routingtable.c
	gcc -Wall -pedantic -std=c99 -g -c sip/routingtable.c -o sip/routingtable.o
sip/sip: common/pkt.o common/seg.o topology/topology.o sip/nbrcosttable.o sip/dvtable.o sip/routingtable.o sip/sip.c 
	gcc -Wall -pedantic -std=c99 -g -pthread sip/nbrcosttable.o sip/dvtable.o sip/routingtable.o common/pkt.o common/seg.o topology/topology.o sip/sip.c -o sip/sip 
client/client: client/Key_Event.c client/Key_Event.h client/Show.c client/Show.h client/Time.c client/Time.h client/commondef.h client/logic.h client/logic.c client/main.c client/message.h client/receive.c client/receive.h client/screen.c client/screen.h common/seg.o client/stcp_client.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread client/Key_Event.c client/Show.c client/Time.c client/logic.c client/main.c client/receive.c client/screen.c common/seg.o client/stcp_client.o topology/topology.o -o client/client 
server/server: server/ser.c server/func.c server/func.h server/message.c server/message.h server/ser.h common/seg.o server/stcp_server.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread server/ser.c server/func.c server/message.c common/seg.o server/stcp_server.o topology/topology.o -o server/server
common/seg.o: common/seg.c common/seg.h
	gcc -Wall -pedantic -std=c99 -g -c common/seg.c -o common/seg.o
client/stcp_client.o: client/stcp_client.c client/stcp_client.h 
	gcc -Wall -pedantic -std=c99 -g -c client/stcp_client.c -o client/stcp_client.o
server/stcp_server.o: server/stcp_server.c server/stcp_server.h
	gcc -Wall -pedantic -std=c99 -g -c server/stcp_server.c -o server/stcp_server.o

clean:
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf son/*.o
	rm -rf son/son
	rm -rf sip/*.o
	rm -rf sip/sip 
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf client/app_simple_client
	rm -rf client/app_stress_client
	rm -rf server/app_simple_server
	rm -rf server/app_stress_server
	rm -rf server/server
	rm -rf client/client
	rm -rf server/receivedtext.txt


.PHONY: git pull push
pull:
	-@git pull origin master
push:
	-@git push origin master
git:	
	-@git add . --ignore-errors &> /dev/null # KEEP IT
	-@git commit $(GITFLAGS) # KEEP IT

playson:
	./son/son

playsip:
	./sip/sip

plays:
	./server/server

playc:
	./client/client
	



