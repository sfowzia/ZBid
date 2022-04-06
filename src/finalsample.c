#ifndef FINALSAMPLE_H
#define FINALSAMPLE_H

#include "finalsample.h"
#include "structs.h"
#include "protocol.h"
//#endif

#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>   
#include "csapp.h"
#include "helpers.h"
#include "sbuf.h"
#include <server.h>

extern sbuf_t sbuf;
extern list_t* usersList;
extern list_t* auctionsList;

extern volatile int auctionID;
extern volatile int numJobThreads;
extern volatile int numTick;
extern char* auctionFile;

volatile int tick = 0;

char* login_buffer;
char* job_buffer;

//create mutexes for data structures
sem_t sbufMutex;
sem_t usersMutex;
sem_t auctionsMutex;
sem_t auctionIdMutex;

//create sigint handler flag
int intFlag = 0;

const char exit_str[] = "exit";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock; 

int total_num_msg = 0;
int listen_fd;

void sigint_handler(int sig)
{
    intFlag = 1;
    printf("\nShutting down server...\n");
    //go through usersList and log everyone out
    P(&usersMutex);
    node_t* current = NULL;
    if(usersList->head != NULL){
        current = usersList->head;
    }
    while(current != NULL){
        users_t* currentUser = (users_t*) (current->data);
        currentUser->loginFlag = 0;
        close(currentUser->fd);
        current = current->next;
    }
    V(&usersMutex);
    //free all malloc?
    free(login_buffer);
    free(job_buffer);
    free(auctionFile);
    //destroy all semaphores
    sem_destroy(&sbufMutex);
    sem_destroy(&usersMutex);
    sem_destroy(&auctionsMutex);
    sem_destroy(&auctionIdMutex);

    //close the listening sockets
    close(listen_fd);


    return;
    //exit(0);
}

int server_init(int server_port){
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt))<0)
    {
	perror("setsockopt");exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);

    return sockfd;
}
//Function running in thread
void *process_client(void* clientfd_ptr)
{  
    //client thread stuff
    //pthread_detach(pthread_self());
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    fd_set read_fds;
    //petr_header* header = (petr_header*)malloc(sizeof(petr_header));

    int logout_check = 0;  /* CHECKS LOGOUT */
    int retval;

    while(1)
    {
        petr_header header;// = (petr_header*)malloc(sizeof(petr_header));
        
        int received_size = rd_msgheader(client_fd, &header);

        if(header.msg_type == 0){
            continue;
        }
      
        // printf("in process client\n");
        // printf("receive size rd_msgheader = %d\n", received_size);
        // printf("client fd = %d\n", client_fd);
 
        job_t* newJob = malloc(sizeof(job_t)); 
        newJob->clientfd = client_fd;
        newJob->jobMsgType = header.msg_type;
        // printf("job message = %d\n", header.msg_type);
        newJob->jobMsgLength = header.msg_len;

        //read message from client
        pthread_mutex_lock(&buffer_lock); //LOCK BUFFER
        job_buffer = calloc(header.msg_len, sizeof(char));
        received_size = 0;
        received_size = read(client_fd, job_buffer, header.msg_len);
        //received_size = read(client_fd, job_buffer, sizeof(job_buffer));
        // printf("job buffer = %s\n", job_buffer);
        // printf("150 receive size = %d\n", received_size);
    
        /// Reading issue ///
        if(received_size < 0)
        {
            printf("Receiving failed\n");
            logout_check = 0;
            pthread_mutex_unlock(&buffer_lock); //UNLOCK BUFFER
            continue;
        }
        else if(received_size >= 0)
        {
            newJob->jobBuffer = malloc(sizeof(header.msg_len));
            strcpy(newJob->jobBuffer, job_buffer);
            pthread_mutex_unlock(&buffer_lock); //UNLOCK BUFFER
            //insert jobs into the job buffer/job queue
            //only wants one person to insert one job at a time to avoid race condition
            // printf("before insert\n");
            sbuf_insert(&sbuf, newJob);
            // printf("after insert\n");
            //V(&sbufMutex);
            if(newJob->jobMsgType == LOGOUT){
                return NULL;
            }
        }
    }
    printf("Close current client connection\n");
    close(client_fd);
    return NULL;
}

void *run_tick(void* num){
    int m = 0;
    if(num){
        m = *((int*)num);
    }

    time_t previousTime = time(NULL);
    time_t currentTime;
    double difference = 0.0;
    
    if(m){
        while(1){
            currentTime = time(NULL);
            difference = difftime(currentTime, previousTime);
            //check this later
            if(difference == m){
                tick++;
                node_t* current = NULL;
                if(auctionsList->head != NULL){
                    current = auctionsList->head;
                    while(current != NULL)
                    {
                        auctions_t* currentAuction = (auctions_t*) (current->data);
                        currentAuction->duration = currentAuction->duration - 1;
                        if(currentAuction->duration == 0){
                            P(&auctionsMutex);
                            closeAuction(currentAuction);
                            int index = findAuctionIndex(currentAuction);
                            removeByIndex(auctionsList, index);
                            V(&auctionsMutex);
                            break;
                        }
                        current = current->next;
                    }
                }
                
                previousTime = time(NULL);
            }
        }
    }
    else{
        //default case
        while(1){
            char* stdinBuffer = NULL;
            size_t size = 0;
            ssize_t result;
            result = getline(&stdinBuffer, &size, stdin);
            if(result){
                printf("Ticked!\n");
                tick++;
                node_t* current = NULL;
                if(auctionsList->head != NULL){
                    current = auctionsList->head;
                    while(current != NULL)
                    {
                        auctions_t* currentAuction = (auctions_t*) (current->data);
                        currentAuction->duration = currentAuction->duration - 1;
                        if(currentAuction->duration == 0){
                            P(&auctionsMutex);
                            closeAuction(currentAuction);
                            int index = findAuctionIndex(currentAuction);
                            removeByIndex(auctionsList, index);
                            V(&auctionsMutex);
                            break;
                        }
                        current = current->next;
                    }
                }
                printf("Current tick number = %d\n", tick);
            }
        }
    }
}

void processJob(job_t* job){
    int n;
    char buffer[BUFFER_SIZE];
    
    int client_fd = job->clientfd;
    int messageType = job->jobMsgType;
    int messageLength = job->jobMsgLength;
    char* messageBuffer = (char*)calloc(messageLength, sizeof(char));
    if(strlen(job->jobBuffer) > 0){
        strcpy(messageBuffer, job->jobBuffer);
    }
    else{
        strcpy(messageBuffer, "");
    }

    char* itemName;
    char* durationStr = NULL;
    char* priceStr = NULL;
    char* bidStr = NULL;
    char* auctionIdStr = NULL;
    int duration, price;
    int auctionId = 0;

    int bid = 0;
    

    switch(messageType){
        case LOGOUT:
            logout(client_fd, usersList);
            break;

        case ANCREATE:
           // printf("in ancreate case\n");
            itemName = strtok(messageBuffer,"\r\n");

            durationStr = strtok(NULL,"\r\n");
            duration = atoi(durationStr);

            priceStr = strtok(NULL,"\r\n");
            price = atoi(priceStr);
            
            P(&auctionsMutex);
            createAuction(client_fd, auctionsList, itemName, duration, price);
            V(&auctionsMutex);
            break;

        case ANCLOSED:
            break;

        case ANLIST:
            P(&auctionsMutex);
            listAuctions(client_fd, auctionsList);
            V(&auctionsMutex);
            break;

        case ANWATCH:
            auctionId = atoi(messageBuffer);
            P(&auctionsMutex);
            watchAuction(client_fd, auctionsList, auctionId);
            V(&auctionsMutex);
            break;

        case ANLEAVE:
            auctionId = atoi(messageBuffer);
            P(&auctionsMutex);
            leaveAuction(client_fd, auctionsList, auctionId);
            V(&auctionsMutex);
            break;

        case ANBID:
            auctionIdStr = strtok(messageBuffer,"\r\n");
            auctionId = atoi(auctionIdStr);
            bidStr = strtok(NULL,"\r\n");
            bid = atoi(bidStr);
            P(&auctionsMutex);
            bidAuction(client_fd, auctionsList, auctionId, bid);
            V(&auctionsMutex);
            break;

        case ANUPDATE:
            //updateAuction();
            break;

        case USRLIST:
            P(&usersMutex);
            printUsers(client_fd);
            V(&usersMutex);
            break;

        case USRWINS:
            P(&usersMutex);
            usersWins(client_fd);
            V(&usersMutex);
            break;

        case USRSALES:
            printf("in get sales\n");
            P(&usersMutex);
            usersSales(client_fd);
            V(&usersMutex);
            break;

        case USRBLNC:
            P(&usersMutex);
            userBalance(client_fd);
            V(&usersMutex);
            break;

        default:
            break;
    }
}

void *run_jobs(){
    pthread_detach(pthread_self());
    while(1){
        job_t* job = sbuf_remove(&sbuf);
        if(job == NULL){
            continue;
        }
        processJob(job);

        //free(job->jobBuffer);
        free(job);
    }
}

void run_server(int server_port){
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    pthread_t tid;
    char* username;
    char* password;

    //sem_init all mutexes created
    sem_init(&sbufMutex, 0, 1);
    sem_init(&usersMutex, 0, 1);
    sem_init(&auctionsMutex, 0, 1);
    sem_init(&auctionIdMutex, 0, 1);
    
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Failed to set signal handler");
        exit(EXIT_FAILURE);
    }

    //sigint_handler might need to be called
    while(1){

        // Wait and Accept the connection from client
        printf("Wait for new client connection\n");
        int* client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA*)&client_addr, (socklen_t*) &client_addr_len);
        if(intFlag){
            return;
        }

        if (*client_fd < 0) {
            printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            //printf("d\n");
            petr_header* header = (petr_header*)malloc(sizeof(petr_header));
            int received_size = 0;
            received_size = rd_msgheader(*client_fd, header);
            if(received_size < 0)
            {
                printf("Receiving failed\n");
                break;
            }
            else if(received_size == 0)
            {
                /************ LOGIN ***************/
                if (header->msg_type == 0x10)
                {
                    pthread_mutex_lock(&buffer_lock);  /*LOCK BUFFER*/
                    login_buffer = calloc(header->msg_len, sizeof(char));
                    read(*client_fd, login_buffer, header->msg_len); /* read from client*/
                    username = strtok(login_buffer, "\r\n");
                    password = strtok(NULL, "\r\n");
                    pthread_mutex_unlock(&buffer_lock);  /*UNLOCK BUFFER*/
                    P(&usersMutex); /*LOCK USERS LIST*/
                    int result = login(*client_fd, usersList, username, password);
                    V(&usersMutex); /*UNLOCK USERS LIST*/
                    if(result == -1){
                        close(*client_fd);
                        continue;
                    }
                    //free(client_fd);
                }
                //create client thread right here
                printf("Client connetion accepted\n");
                pthread_create(&tid, NULL, process_client, (void *)client_fd);
                //free(client_fd);
            }
            //free(header);
        }
        //free(client_fd);
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

#endif