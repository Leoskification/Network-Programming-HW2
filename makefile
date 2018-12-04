all:
	gcc -pthread -o server server.c
	gcc -o client client.c
