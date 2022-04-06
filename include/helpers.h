#ifndef HELPERS_H
#define HELPERS_H

#include "finalsample.h"
#include "linkedlist.h"
#include "protocol.h"
#include "server.h"
#include "structs.h"
#include <semaphore.h>
#include "sbuf.h"

extern sbuf_t sbuf;
extern list_t* usersList;
extern list_t* auctionsList;

extern volatile int auctionID;
extern volatile int numJobThreads;
extern volatile int numTick;
extern char* auctionFile;
extern volatile int tick;

void displayHelpMenu();
users_t* findUsername(int client_fd);
users_t* findUserStruct(char* username);
int findClientFd(char* username);
int findAuctionIndex(auctions_t* auction);
int findUserIndex(list_t* list, char* username);
int login(int client_fd, list_t* users, char* username, char* password);
void logout(int client_fd, list_t* users);
void createAuction(int client_fd, list_t* auctions, char* itemName, int duration, int price);
void closeAuction(auctions_t* auction);
void listAuctions(int client_fd, list_t* auctions);
void auctionInfo(auctions_t* auction, char* str);
void watchAuction(int client_fd, list_t* auctions, int auctionId);
void leaveAuction(int client_fd, list_t* auctions, int auctionId);
void updateAuction(auctions_t* auction, char* fromUsername);
void bidAuction(int client_fd, list_t* auctions, int auctionId, int bid);
void printUsers(int client_fd);
void userBalance(int client_fd);
void usersWins(int client_fd);
void usersWinsInfo(auctions_t* auctions, char* str);
void usersSales(int client_fd);
void usersSalesInfo(auctions_t* auctions, char* str);


#endif