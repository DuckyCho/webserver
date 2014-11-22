CC = gcc
OBJS = webserv.c

all : webServer

webServer : $(OBJS)
	$(CC) -pthread -o webServer $(OBJS)
