all:
	gcc -c client.c
	gcc -o client client.o -lpthread
	gcc -c fileShareServer.c
	gcc -o fileShareServer fileShareServer.o -lpthread
	hostname -I

