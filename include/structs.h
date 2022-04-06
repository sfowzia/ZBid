#ifndef STRUCTS_H
#define STRUCTS_H

#include "linkedlist.h"
#include "protocol.h"
#include <semaphore.h>
#include "sbuf.h"

// extern sbuf_t sbuf;
// extern list_t* usersList;
// extern list_t* auctionsList;

extern volatile int auctionID;
extern volatile int numJobThreads;
extern volatile int numTick;
extern char* auctionFile;
extern volatile int tick;

typedef struct users{
    char* username;
    char* password;
    int loginFlag;
    int balance;
    //file descriptor: used to receive and send messages between users and server
    int fd; 
    //store the auctions' ids of the auctions that the user creates
    list_t* auctionsOwned;
    //store the auctions' ids of the auctions that the user wins
    list_t* auctionsWon;
    list_t* auctionsSold;
} users_t;

typedef struct auctions{
    int id;
    char* name;
    int duration;
    int bin_price;
    int bin_flag;
    int bid;
    char* currentWinner;
    //username of the user who creates this auction
    char* creator;
    //stores the usernames of the watchers
    list_t* watchers;
    int numberOfWatchers;
} auctions_t;


#endif