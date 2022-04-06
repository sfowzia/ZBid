#ifndef FINALSAMPLE_H
#define FINALSAMPLE_H

#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include "csapp.h"
#include "sbuf.h"

#define BUFFER_SIZE 1024
#define SA struct sockaddr

// extern sbuf_t sbuf;
// extern list_t* usersList;
// extern list_t* auctionsList;

extern volatile int auctionID;
extern volatile int numJobThreads;
extern volatile int numTick;
extern char* auctionFile;
extern volatile int tick;

void P(sem_t* s);
void V(sem_t* s);
//void process_client(void* clientfd_ptr)
void run_server(int server_port);
//void run_client(char *server_addr_str, int server_port);
void *run_tick(void* num);
void *run_jobs();
#endif