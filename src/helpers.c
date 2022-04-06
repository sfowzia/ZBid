#include "finalsample.h"
#include "linkedlist.h"
#include "helpers.h"
#include "protocol.h"
#include "server.h"
#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "sbuf.h"

extern sbuf_t sbuf;
extern list_t* usersList;
extern list_t* auctionsList;
extern volatile int tick;
extern volatile int auctionID;
extern volatile int numJobThreads;
extern volatile int numTick;
extern char* auctionFile;
extern volatile int tick;

extern sem_t sbufMutex;
extern sem_t usersMutex;
extern sem_t auctionsMutex;
extern sem_t auctionIdMutex;
extern int intFlag;

void displayHelpMenu(){
    printf("./bin/zbid_server [-h] [-j N] [-t M] PORT_NUMBER AUCTION_FILENAME\n\n");

    printf("-h\t\tDisplays this help menu, and returns EXIT_SUCCESS.\n");

    printf("-j N\t\tNumber of job threads. If option not specified, default to 2.\n");

    printf("-t M\t\tM seconds between time ticks. If option not specified,\n");
    printf("\t\tdefault is to be in debug mode and to only tick upon\n");
    printf("\t\tinput from stdin e.g. a newline is entered then the server ticks once.\n");
    
    printf("PORT_NUMBER\tPort number to listen on.\n");

    printf("AUCTION_FILE\tFile to read auction item information from at the start ");
    printf("of the server.\n");
}

users_t* findUsername(int client_fd){
    if(usersList->head == NULL){
        return NULL;
    }
    node_t* current = usersList->head;
    while(current != NULL){
        users_t* currentUser = (users_t*) (current->data);
        if(currentUser->fd == client_fd){
            return currentUser;
        }
        current = current->next;
    }
    return NULL;
}

users_t* findUserStruct(char* username){
    printf("in find struct\n");
    printf("username = %s\n", username);
    if(usersList->head == NULL){
        return NULL;
    }
    printf("a\n");
    node_t* current = usersList->head;
    while(current != NULL){
        printf("b\n");
        users_t* currentUser = (users_t*) (current->data);
        printf("c\n");
        printf("username = %s\n", username);
        printf("current user = %s", currentUser->username);
        if(strcmp(username, currentUser->username) == 0){
            printf("d\n");
            return currentUser;
        }
        current = current->next;
    }
    return NULL;
}

int findClientFd(char* username){
    if(usersList->head == NULL){
        return -1;
    }
    node_t* current = usersList->head;
    while(current != NULL){
        users_t* currentUser = (users_t*) (current->data);
        printf("61current user = %s\n", currentUser->username);
        printf("user = %s\n", username);
        if(strcmp(username, currentUser->username) == 0){
            printf("usernames match!\n");
            return currentUser->fd;
            
        }
        current = current->next;
    }
    return -1;
}

int findAuctionIndex(auctions_t* auction){
    //printf("in find index\n");
    int result = -1;
    node_t* current = NULL;
    if(auctionsList->head != NULL){
        current = auctionsList->head;
    }
    while(current != NULL){
        result++;
        auctions_t* currentAuction = (auctions_t*) (current->data);
        if(auction->id == currentAuction->id){
            return result;
        }
        current = current->next;
    }
    return -1;
}

int findUserIndex(list_t* list, char* username){
    printf("in find user index\n");
    int result = -1;
    node_t* current = NULL;
    if(list->head != NULL){
        current = list->head;
    }
    else{
        return result;
    }

    while (current != NULL){
        result++;
        char* currentUser = (char*) (current->data);
        if(strcmp(username, currentUser) == 0){
            return result;
        }
        current = current->next;
    }
    return -1;
}

//LOGIN
int login(int client_fd, list_t* users, char* username, char* password){
    //printf("in login\n");
    node_t* current = users->head;
    node_t* tail = current;

    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];

    while (current != NULL){
        users_t * currentUser = (users_t*) (current->data);
        if(strcmp(username, currentUser->username) == 0){
            //user exists and is currently logged in => return EUSRLGDIN
            if(currentUser->loginFlag){
                //printf("login on\n");
                header->msg_len = 0;
                header->msg_type = EUSRLGDIN;
                wr_msg(client_fd, header, randBuffer);
                return 0;
                //return EUSRLGDIN;
            }
            else{
                if(strcmp(password, currentUser->password) == 0){
                    //user exists, not logged in, and right password => return OK
                    currentUser->loginFlag = 1;
                    header->msg_type = OK;
                    header->msg_len = 0;
                    currentUser->fd = client_fd;
                    wr_msg(client_fd, header, randBuffer);
                    return 0;
                    //return OK;
                }
                else{
                    //user exists, not logged in, but wrong password => return EWRNGPWD
                    header->msg_len = 0;
                    header->msg_type = EWRNGPWD;
                    wr_msg(client_fd, header, randBuffer);
                    return -1;
                    //return EWRNGPWD;
                }
            }
        }
        tail = current;
        current = current->next;
    }
    //create a new account => return OK
    users_t* newUser = (users_t*)malloc(sizeof(users_t));
    //check for semicolons later!!
    newUser->username = username;
    newUser->password = password;
    newUser->fd = client_fd;
    newUser->loginFlag = 1;
    newUser->balance = 0;
    newUser->auctionsOwned = CreateList(NULL, NULL, NULL);
    newUser->auctionsWon = CreateList(NULL, NULL, NULL);
    InsertAtTail(users, (void*) newUser);

    header->msg_type = OK;
    header->msg_len = 0;
    wr_msg(client_fd, header, randBuffer);
    //return OK;
    //free petr_header
    //free(header);
    return 0;
}

//LOGOUT
void logout(int client_fd, list_t* users){
    //printf("in logout\n");
    node_t* current = NULL;
    if(users->head != NULL){
        current = users->head;
    }
    node_t* tail = current;

    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];
    //printf("client fd = %d\n", client_fd);

    while(current != NULL){
        users_t * currentUser = (users_t*) (current->data);
        if(currentUser->fd == client_fd){
            //printf("current client fd = %d\n", currentUser->fd);
            currentUser->loginFlag = 0;
            header->msg_type = OK;
            header->msg_len = 0;
            // printf("client fd = %d\n", client_fd);
            wr_msg(client_fd, header, randBuffer);
            //close(client_fd);
            return;
        }
        tail = current;
        current = current->next;
    }
    return;
    //free petr_header
    //free(header);
}

//ANCREATE
void createAuction(int client_fd, list_t* auctions, char* itemName, int duration, int price){
    //printf("in create auction function\n");
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];
    //errors checking
    if(strcmp(itemName, "") == 0){
        header->msg_len = 0;
        header->msg_type = EINVALIDARG;
        wr_msg(client_fd, header, randBuffer);
        return;
        //return EINVALIDARG;
    }
    if(duration < 1){
        header->msg_len = 0;
        header->msg_type = EINVALIDARG;
        wr_msg(client_fd, header, randBuffer);
        return;
        //return EINVALIDARG;
    }
    if(price < 0){
        header->msg_len = 0;
        header->msg_type = EINVALIDARG;
        wr_msg(client_fd, header, randBuffer);
        return;
        //return EINVALIDARG;
    }

    //printf("1\n");
    auctions_t* newAuction = (auctions_t*)malloc(sizeof(auctions_t));
    //printf("2\n");
    newAuction->name = itemName;
    if(price){
        newAuction->bin_flag = 1;
    }
    else{
        newAuction->bin_flag = 0;
    }
    newAuction->bin_price = price;
    newAuction->duration = duration;
    newAuction->bid = 0;
    newAuction->currentWinner = NULL;
    newAuction->watchers = CreateList(NULL, NULL, NULL);
   
    //set the newAuctions ID!! mutex stuff

    /********* Check & Compare to get UserName *******/
    //findUsername(client_fd, newAuction->creator);
    node_t* current = NULL;
    if(usersList->head != NULL){
        current = usersList->head;
    }
    node_t* tail = current;
    //mutex around here?
    P(&auctionIdMutex);
    auctionID = auctionID + 1;
    newAuction->id = auctionID;
    V(&auctionIdMutex);
    int* thisAuctionId = &(newAuction->id);
    //mutex around here?
    while(current != NULL)
    {
        users_t* currentUser = (users_t*)(current->data);
        if(currentUser->fd == client_fd)
        {
            newAuction->creator = currentUser->username;
            //strcpy(newAuction->creator, currentUser->username);
            InsertAtTail(currentUser->auctionsOwned, (void*) thisAuctionId);
        }
        tail = current;
        current = current->next;
    }
  
    InsertAtTail(auctions, (void*) newAuction);
    //copy auction info into buffer and return it with ANCREATE
    sprintf(randBuffer, "%d", newAuction->id);
    strcat(randBuffer, "\0");
    int size = strlen(randBuffer) + 1;
    header->msg_len = size;
    //server returns message with auction ID
    header->msg_type = ANCREATE;
    wr_msg(client_fd, header, randBuffer);

    //free petr_header
    //free(header);
}
//ANCLOSED
void closeAuction(auctions_t* auction){
    //printf("in closeAuction\n");
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];

    char winPrice[100];
    sprintf(randBuffer, "%d", auction->id);
    strcat(randBuffer,"\r\n");
    //printf("current Winner = %s\n", auction->currentWinner);
    if(auction->currentWinner == NULL){
        strcat(randBuffer,"\r\n");
        strcat(randBuffer,"\r\n");
    }
    else{
        strcat(randBuffer, auction->currentWinner);
        strcat(randBuffer,"\r\n");
        sprintf(winPrice, "%d", auction->bid);
        strcat(randBuffer, winPrice);
        //decrease the balance of the winner
        P(&usersMutex);
        users_t* winner = findUserStruct(auction->currentWinner);
        winner->balance = winner->balance - (auction->bid);
        int* thisAuctionId = &(auction->id);
        InsertAtTail(winner->auctionsWon, (void*) thisAuctionId);
        V(&usersMutex);
    }
    strcat(randBuffer, "\0");
    int size = strlen(randBuffer) + 1;
    header->msg_len = size;
    header->msg_type = ANCLOSED;

    //increase the balance of the creator
    P(&usersMutex);
    users_t* creator = findUserStruct(auction->creator);
    //printf("creator name = %s\n", creator->username);
    // printf("creator balance = %d\n", creator->balance);
    // printf("auction bid = %d\n", auction->bid);
    creator->balance = creator->balance + (auction->bid);
    V(&usersMutex);

    int client_fd = 0;
    node_t* current = NULL;
    if(auction->watchers->head != NULL){
        current = auction->watchers->head;
    }
    while(current != NULL){
        client_fd = 0;
        char* name = (char*) (current->data);
        client_fd = findClientFd(name);
        wr_msg(client_fd, header, randBuffer);
        current = current->next;
    }
}

//ANLIST
void auctionInfo(auctions_t* auction, char* str){
 
    char idStr[100];
    char binStr[100];
    char bidStr[100];
    char numWatchStr[100];
    char durationStr[100];

    //convert int fields of auctions_t into string
    sprintf(idStr, "%d", auction->id);
    sprintf(binStr, "%d", auction->bin_price);
    sprintf(bidStr, "%d", auction->bid);
    sprintf(numWatchStr, "%d", auction->numberOfWatchers);
    sprintf(durationStr, "%d", auction->duration);

    //make the result string by concat the str fields above separating by ;
    strcat(str, idStr);
    strcat(str, ";");
    strcat(str, auction->name);
    strcat(str, ";");
    strcat(str, binStr);
    strcat(str, ";");
    strcat(str, numWatchStr);
    strcat(str, ";");
    strcat(str, bidStr);
    strcat(str, ";");
    strcat(str, durationStr);
    strcat(str, "\n");
}

//ANLIST
void listAuctions(int client_fd, list_t* auctions){
    // printf("in anlist\n");
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char listBuffer[BUFFER_SIZE];
    listBuffer[0] = '\0';
    if(auctions->head == NULL){
        header->msg_len = 0;
        header->msg_type = ANLIST;
        wr_msg(client_fd, header, listBuffer);
        
        //free(header);
        return;
    }
    node_t* current = auctions->head;
    int i = 0;
    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        auctionInfo(currentAuction, listBuffer);
        current = current->next;
        i++;
    }
    strcat(listBuffer, "\0");
    int size = strlen(listBuffer) + 1;
    // printf("size of buffer = %d\n", size);
    header->msg_len = size;
    header->msg_type = ANLIST;
    wr_msg(client_fd, header, listBuffer);
    
    //free(header);
}

//ANWATCH
void watchAuction(int client_fd, list_t* auctions, int auctionId){
  
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];
    randBuffer[0] = '\0';
    int size = 0;

    node_t* current = NULL;
    if(auctions->head != NULL){
        current = auctions->head;
    }
    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        if(currentAuction->id == auctionId){
            currentAuction->numberOfWatchers = currentAuction->numberOfWatchers + 1;
            //look through userList to find the corresponding username
            users_t* user = findUsername(client_fd);
            char* watcherName = user->username;
            // printf("watcher name = %s\n", watcherName);
         
            InsertAtTail(currentAuction->watchers, (void*) watcherName);

            //return ANWATCH with buffer including item name and bin price
            char binStr[100];
            sprintf(binStr, "%d", currentAuction->bin_price);
            strcat(randBuffer, currentAuction->name);
            strcat(randBuffer, "\r\n");
            strcat(randBuffer, binStr);
            strcat(randBuffer, "\0");
            size = strlen(randBuffer) + 1;

            header->msg_len = size;
            header->msg_type = ANWATCH;
            wr_msg(client_fd, header, randBuffer);
            free(header);
            return;
        }
        current = current->next;
    }
    //return EANNOTFOUND;
    header->msg_len = 0;
    header->msg_type = EANNOTFOUND;
    wr_msg(client_fd, header, randBuffer);
    //free(header);
}

//ANLEAVE
void leaveAuction(int client_fd, list_t* auctions, int auctionId){
    int auctionExists = 0;
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];

    //find the username of the user that has the same client_fd 
    //to remove from watchers list in auctionsList
    users_t* user = findUsername(client_fd);
    char* username = user->username;

    node_t* current = NULL;
    current = NULL;
    if(auctions->head != NULL){
        current = auctions->head;
    }

    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        if(currentAuction->id == auctionId){
            auctionExists = 1;
            currentAuction->numberOfWatchers = currentAuction->numberOfWatchers - 1;
            //remove username from the watchers list -> has to write remove function in linkedlist
            int index = findUserIndex(currentAuction->watchers, username);
            removeByIndex(currentAuction->watchers, index);
            break;
        }
        current = current->next;
    }
    if(!auctionExists){
        header->msg_len = 0;
        header->msg_type = EANNOTFOUND;
        wr_msg(client_fd, header, randBuffer);
    }
    else{
        header->msg_type = OK;
        header->msg_len = 0;
        wr_msg(client_fd, header, randBuffer);
    }
}

//ANUPDATE
void updateAuction(auctions_t* auction, char* fromUsername){
    // printf("in an update\n");

    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    header->msg_type = ANUPDATE;
    char randBuffer[BUFFER_SIZE];
    char bidStr[100];
    sprintf(randBuffer, "%d", auction->id);
    //strcat(randBuffer, auction->id);
    strcat(randBuffer, "\r\n");
    strcat(randBuffer, auction->name);
    strcat(randBuffer, "\r\n");
    strcat(randBuffer, fromUsername);
    strcat(randBuffer, "\r\n");
    sprintf(bidStr,"%d", auction->bid);
    strcat(randBuffer, bidStr);
    strcat(randBuffer, "\0");
    int size = strlen(randBuffer) + 1;
    header->msg_len = size;

    node_t* current = NULL;
    if(auction->watchers->head != NULL){
        current = auction->watchers->head;
    }

    while(current != NULL){
        char* username = (char*) (current->data);
        int client_fd = findClientFd(username);
        // printf("client fd = %d\n", client_fd);
        //if(client_fd != -1 && strcmp(username, auction->creator) != 0){
        if(client_fd != -1){
            wr_msg(client_fd, header, randBuffer);
        }
        current = current->next;
    }
}

//ANBID
void bidAuction(int client_fd, list_t* auctions, int auctionId, int bid){
    printf("in anbid\n");
    int auctionExists = 0;
    users_t* user = findUsername(client_fd);
    char* username = user->username;
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];
   
    if(auctionsList->head == NULL){
        header->msg_len = 0;
        header->msg_type = EANNOTFOUND;
        wr_msg(client_fd, header, randBuffer);
        return;
    }
    node_t* current = auctionsList->head;
    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        if(currentAuction->id == auctionId){
            // printf("while id is match\n");
            auctionExists = 1;
            //check if the creator of the auction is the user who bids
            //if(strcmp(currentAuction->creator, username) == 0){
            if(currentAuction->creator == username){
                // printf("while 3\n");
                header->msg_len = 0;
                header->msg_type = EANDENIED;
                wr_msg(client_fd, header, randBuffer);
                return;
            }
            //check if the user who bids is watching the auction
            int userWatches = 0;
            node_t* curr = NULL;
            if(currentAuction->watchers->head != NULL){
                curr = currentAuction->watchers->head;
            }
            while(curr != NULL){
                char* currentUser = (char*) (curr->data);
                // printf("current user = %s\n", currentUser);
                // printf("username = %s\n", username);
                if(strcmp(currentUser, username) == 0){
                //if(currentUser->username == username){
                    // printf("while 8\n");
                    userWatches = 1;
                    //update auction data structure
                    //check if bid exceeds bin_price
                    if(currentAuction->bin_flag == 1 && bid > currentAuction->bin_price){
                        currentAuction->bid = bid;
                        currentAuction->currentWinner = username;
                        closeAuction(currentAuction);
                        int index = findAuctionIndex(currentAuction);
                        removeByIndex(auctionsList, index);
                        return;
                    }
                    //check if bid exceeds current highest bid
                    printf("highest bid = %d\n", currentAuction->bid);
                    printf("bid = %d\n", bid);
                    if(bid > currentAuction->bid){
                        currentAuction->bid = bid;
                        currentAuction->currentWinner = username;
                        //strcpy(currentAuction->currentWinner, username);
                        //CALL ANUPDATE to all users watching this auction
                        P(&auctionIdMutex);
                        updateAuction(currentAuction, username);
                        V(&auctionIdMutex);
                        //return OK
                        header->msg_type = OK;
                        header->msg_len = 0;
                        wr_msg(client_fd, header, randBuffer);
                        return;
                    }
                    else{
                        header->msg_type = EBIDLOW;
                        header->msg_len = 0;
                        wr_msg(client_fd, header, randBuffer);
                        return;
                    }

                }
                curr = curr->next;
            }
            if(!userWatches){
                header->msg_len = 0;
                header->msg_type = EANDENIED;
                wr_msg(client_fd, header, randBuffer);
                return;
            }
        }
        current = current->next;
    }
}

//USRLIST
void printUsers(int client_fd){
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    header->msg_len = BUFFER_SIZE;
    char randBuffer[BUFFER_SIZE];

    node_t* current = usersList->head;
    node_t* tail = current;
    while(current != NULL){
        users_t * currentUser = (users_t*) (current->data);
        if(currentUser->fd != client_fd){
            strcat(randBuffer, currentUser->username);
            strcat(randBuffer, "\n");
        }
        tail = current;
        current = current->next;
    }
    strcat(randBuffer, "\0");
    int size = strlen(randBuffer) + 1;

    header->msg_len = size;
    header->msg_type = USRLIST;
    wr_msg(client_fd, header, randBuffer);
    //free(header);
}

void userBalance(int client_fd)
{
    users_t * user = findUsername(client_fd);
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    char bufferr[BUFFER_SIZE];
    char balance[100];
    balance[0] = '\0';
    bufferr[0] = '\0';
    printf("in balance\n");
    printf("user balance is %d\n", user->balance);
    sprintf(balance, "%d", user->balance);
    printf("balance string = %s\n", balance);
    strcat(bufferr, balance);
    strcat(bufferr, "\0");
    printf("buffer = %s\n", bufferr);
    int size = strlen(bufferr) + 1;

    header->msg_len = size;
    header->msg_type = USRBLNC;
    wr_msg(client_fd, header, bufferr);
}

// USRWINS - helper function
void usersWinsInfo(auctions_t* auctions, char* str)
{
    char idStr[100];
    char bidStr[100];

    //convert int fields of list into string to be able to concatenate
    sprintf(idStr, "%d", auctions->id);
    sprintf(bidStr, "%d", auctions->bid);
    
    /******************************FORMATTING******************************/
    //make the result string by concat the str fields above & separating by ;
    strcat(str, idStr);
    strcat(str, ";");
    
    strcat(str, auctions->name);
    strcat(str, ";");

    strcat(str, bidStr);
    strcat(str, "\n\0");

}

//USRWINS
void usersWins(int client_fd){

    /***************************INITIALIZE****************************/
    list_t* list = NULL;
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    node_t* current = NULL;
    char userBuffer[BUFFER_SIZE];
    userBuffer[0] = '\0';
    /*****************************************************************/

    if(usersList->head != NULL){
        current = usersList->head;
    }

    while(current != NULL){
        //auctions_t* currentAuction = (auctions_t*) (current->data);
        users_t* currentUser = (users_t*) (current->data);
        if(currentUser->fd == client_fd){
            list = currentUser->auctionsWon;
            break;
        }
        current = current->next;
    }

    current = NULL;
    if(list->head != NULL){
        current = list->head;
    }
    else{
        header->msg_len = 0;
        header->msg_type = USRWINS;
        wr_msg(client_fd, header, userBuffer);
        return;
    }

    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        usersWinsInfo(currentAuction, userBuffer);
        current = current->next;
    }
    int size = strlen(userBuffer) + 1;
    header->msg_len = size;
    header->msg_type = USRWINS;
    wr_msg(client_fd, header, userBuffer);
    return;
    

}

//USRSALES - helper function
void usersSalesInfo(auctions_t* auctions, char* str)
{
    char idStr[100];
    char bidStr[100];

    //convert int fields of list into string to be able to concatenate
    sprintf(idStr, "%d", auctions->id);
    if(auctions->currentWinner != NULL){
        sprintf(bidStr, "%d", auctions->bid);
    }
    
    /******************************FORMATTING******************************/
    //make the result string by concat the str fields above & separating by ;
    strcat(str, idStr);
    strcat(str, ";");
    
    strcat(str, auctions->name);
    strcat(str, ";");

    if(auctions->currentWinner != NULL){
        strcat(str, auctions->currentWinner);
        strcat(str, ";");

        strcat(str, bidStr);
        strcat(str, "\n\0");
    }
    else{
        strcat(str, "None");
        strcat(str, ";");

        strcat(str, "None");
        strcat(str, "\n\0");
    }

}

//USRSALES
void usersSales(int client_fd){

    /***************************INITIALIZE****************************/
    list_t* list = NULL;
    petr_header* header = (petr_header*)malloc(sizeof(petr_header));
    node_t* current = NULL;
    char userBuffer[BUFFER_SIZE];
    userBuffer[0] = '\0';
    //segfault line below, you accessed current = NULL
    //auctions_t* currentAuction = (auctions_t*) (current->data);
    /*****************************************************************/

    if(usersList->head != NULL){
        current = usersList->head;
    }
    
    while(current != NULL){
        users_t* currentUser = (users_t*) (current->data);
        if(currentUser->fd == client_fd){
            //list = currentUser->auctionsSold;
            //auctionsOwned is auctionsSold, so you dont have to make another list
            list = currentUser->auctionsOwned;
            break;
        }
        //usersSalesInfo(currentAuction, userBuffer);
        current = current->next;
    }

    current = NULL;
    if(list->head != NULL){
        current = list->head;
    }
    else{
        header->msg_len = 0;
        header->msg_type = USRSALES;
        wr_msg(client_fd, header, userBuffer);
        return;
    }

    while(current != NULL){
        auctions_t* currentAuction = (auctions_t*) (current->data);
        usersSalesInfo(currentAuction, userBuffer);
        current = current->next;
    }
    int size = strlen(userBuffer) + 1;
    header->msg_len = size;
    header->msg_type = USRSALES;
    wr_msg(client_fd, header, userBuffer);
    return;
}


// void readAuctionFile()
// {

// }

