#include <pthread.h>
#include <signal.h>

#include "finalsample.h"
#include "linkedlist.h"
#include "helpers.h"
#include "protocol.h"
#include "server.h"
#include "structs.h"
#include <semaphore.h>
#include "sbuf.h"

//shared job buffer among the threads
sbuf_t sbuf;
list_t* usersList;
list_t* auctionsList;
volatile int auctionID = 0;
volatile int numJobThreads = 0;
volatile int numTick = 0;
char* auctionFile;

extern sem_t sbufMutex;
extern sem_t usersMutex;
extern sem_t auctionsMutex;
extern sem_t auctionIdMutex;
extern int intFlag;

extern volatile int tick;

int main(int argc, char* argv[])
{
    int opt;
    usersList = CreateList(NULL, NULL, NULL);
    auctionsList = CreateList(NULL, NULL, NULL);
    int j = 0;
    int t = 0;
    pthread_t tid;

    unsigned int port = 0;
    while ((opt = getopt(argc, argv, "hjt:")) != -1) {
        switch (opt) {
        case 'h':
            displayHelpMenu();
            exit(EXIT_SUCCESS);
            break;
        case 'j':
            j = 1;
            numJobThreads = atoi(optarg);
            break;
        case 't':
            numTick = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    //read port number
    port = atoi(argv[argc - 2]);
    if (port == 0){
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
        exit(EXIT_FAILURE);
    }

    //read auction1.txt file
    auctionFile = calloc(100 ,sizeof(char));
    strcpy(auctionFile, argv[argc - 1]);

    //if option j is not specified, the default value for numJobThreads is 2
    if(!j){
        numJobThreads = 2;
    }
    //if option t is not specified, the default value is in debug mode

    //initialize the job buffer/job queue for job threads
    // printf("1\n");
    sbuf_init(&sbuf, BUFFER_SIZE); //check SBUFSIZE
    // printf("2\n");
    //create job threads
    int i;
    // printf("3\n");
    for(i = 0; i < numJobThreads; i++){
        pthread_create(&tid, NULL, run_jobs, NULL);
    }
    // printf("4\n");
    //create tick thread
    // printf("5\n");
    if(t){
        // printf("a\n");
        int i = numTick;
        pthread_create(&tid,NULL, run_tick, &i);
    }
    else{
        // printf("b\n");
        //default case of tick thread
        pthread_create(&tid,NULL, run_tick, NULL);
    }
    // printf("6\n");

    //then run_server
    run_server(port);
    exit(0);

    // while ((opt = getopt(argc, argv, "p:")) != -1) {
    //     switch (opt) {
    //     case 'p':
    //         port = atoi(optarg);
    //         break;
    //     default: /* '?' */
    //         fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
    //                 argv[0]);
    //         exit(EXIT_FAILURE);
    //     }
    // }


    return 0;
}

// int main() {

//     // int i;
//     // int listenfd, *connfdp, port;
//     // pthread_t tid;
//     // socklen_t clientlen;
//     // struct sockaddr_storage clientaddr;

//     // listenfd = Open_listenfd(argv[1]);
//     // while(1)
//     // {
//     //     clientlen = sizeof();
//     //     connfdp = Malloc(sizeof(int));
//     //     connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
//     //     Pthread_create(&tid, NULL, thread, &connfdp);
//     // }

//     // You got this!
//     return 0;
// }

