#include "sbuf.h"

#include "csapp.h"
#include <semaphore.h>
#include <pthread.h>

/*Create an empty, bounded, shared buffer with n slots */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(int));
    sp->n = n;                      /* Buffer holds max of n items */
    sp->front = sp -> rear = 0;     /* Empty buffer iff front  == read */
    Sem_init(&sp->mutex, 0, 1);     /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);     /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);     /* Initially, buf has zero data items  */
}

//change from in item to job_t* item to fit our project
void sbuf_insert(sbuf_t *sp, job_t* item)
{
    P(&sp->slots);
    P(&sp->mutex);
    // sem_wait(&sp->slots);
    // sem_wait(&sp->mutex);
    //printf("here1\n");
    sp->buf[(++sp->rear)%(sp->n)] = item;
    //printf("here2\n");
    V(&sp->mutex);
    V(&sp->items); 

}

job_t* sbuf_remove(sbuf_t *sp)
{
    //printf("in remove\n");
    job_t* item;
    //printf("before lock\n");
    P(&sp->items);
    P(&sp->mutex);
    //printf("sp size = %d\n", sp->n);
    if(sp->n == 0){
        return NULL;
    }
    item = sp->buf[(++sp->front) % (sp->n)];
    //printf("before unlock\n");
    V(&sp->mutex);
    V(&sp->slots);
    //printf("after unlock\n");
    return item;
}