#ifndef SBUF_H
#define SBUF_H

#include "finalsample.h"
#include "linkedlist.h"
#include "protocol.h"
#include "server.h"
#include "structs.h"
#include <semaphore.h>
#include <pthread.h>
#include "csapp.h"

typedef struct{
    char* jobBuffer;
    int jobMsgType;
    int jobMsgLength;
    int clientfd;
}job_t;

//change the buffer array type from int* buf to job_t** to fit our project
typedef struct{
    job_t** buf;       /* Buffer array */
    int n;          /* Maximum number of slots */
    int front;      /* buf[(front+1)%n] is the first item */
    int rear;       /* buf[rear%n] is last item */
    sem_t mutex;    /* Protects access to buf */
    sem_t slots;    /* Counts available slots */
    sem_t items;    /* Counts available items */
}sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_insert(sbuf_t *sp, job_t* item);
job_t* sbuf_remove(sbuf_t *sp);

#endif