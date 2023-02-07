#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "stringmap.h"


typedef struct ClientData {
    // sockfd is key
    int sockfd;
    StringMap *msgRoot;
} ClientData;

typedef struct MsgData {
    char *topic;
    char *msgSentBy;
    char *msg;
} MsgData;

typedef struct StatsData {
    int connCli;
    int disconnCli;
    int pubCount;
    int subCount;
    int unsubCount;
} StatsData;

// clientRoot - variable to store all clients and related data associated
// to client.
// topicRoot - variable to store all topics and related data associated
// to topics.
StringMap *clientRoot, *topicRoot;
// store statistics of transactions. 
StatsData *statsData;

void show_stats(int signal);
void *handle_connection(void *socketPtr);
void form_and_send_msg(int sockfd, char *sentBy, char *topic, char *msg);
void handle_command(int sockfd, char *buffer);
int init_socket(int port);
void print_topic_tree();
void print_client_tree();
void print_names_only();
void process_name(int sockfd, char *command);
void process_sub(int sockfd, char *command);
void process_unsub(int sockfd, char *command);
void process_pub(int sockfd, char *command);

//**********************************************************
// Prints stats when SIGHUP is initiated. It takes data 
// that is stored in global structure that is keeping 
// track of transactions. It is printed
//**********************************************************
void show_stats(int signal) {
    fprintf(stderr, "Connected clients:%d\n"
            , statsData->connCli - statsData->disconnCli);
    fprintf(stderr, "Completed clients:%d\n",statsData->disconnCli);
    fprintf(stderr, "pub operations:%d\n",statsData->pubCount);
    fprintf(stderr, "sub operations:%d\n",statsData->subCount);
    fprintf(stderr, "unsub operations:%d\n",statsData->unsubCount);
    fflush(stderr);
}

// **********************************************************************
// Validate names and other parameters recieved as part of commands
// **********************************************************************
int valid_name(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            continue;
        }
        if (str[i] >= 'A' && str[i] <= 'Z') {
            continue;
        }
        if (str[i] >= '0' && str[i] <= '9') {
            continue;
        }
        if (str[i] == ' ' || str[i] == ':'
                || str[i] == '\n') {
            return 1;
        }
    }
    if (strlen(str) == 0) {
        return 1;
    }
    return 0;
}

// **********************************************************************
// Validate if a string contains only digits
// **********************************************************************
int is_numeric(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 1;
        }
    }
    return 0;
}

// **********************************************************************
// Validate parms sent as arguments
// **********************************************************************
int check_parms(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: psserver connections [portnum]\n");
        fflush(stderr);
        return 1;
    }

    if (is_numeric(argv[1])) {
        fprintf(stderr, "Usage: psserver connections [portnum]\n");
        fflush(stderr);
        return 1;
    }
    int maxConn = atoi(argv[1]);
    if (maxConn < 0) {
        fprintf(stderr, "Usage: psserver connections [portnum]\n");
        fflush(stderr);
        return 1;
    }

    int mainPort = 0;
    if (argc == 3) {
        if (is_numeric(argv[2])) {
            fprintf(stderr, "Usage: psserver connections [portnum]\n");
            fflush(stderr);
            return 1;
        }
        mainPort = atoi(argv[2]);
        if ((mainPort < 1024 && mainPort != 0) || mainPort > 65535) {
            fprintf(stderr, "Usage: psserver connections [portnum]\n");
            fflush(stderr);
            return 1;
        }
    }
    return 0;
}

// **********************************************************************
// initiate all global varialbles
// **********************************************************************
void init_global_var(){

    clientRoot = stringmap_init();
    topicRoot = stringmap_init();
    statsData = malloc(sizeof(StatsData));
    statsData->connCli = 0;
    statsData->disconnCli = 0;
    statsData->pubCount = 0;
    statsData->subCount = 0;
    statsData->unsubCount = 0;
}

// **********************************************************************
// Main funciton. This function initiates sockets, accepts connections
// from clients and starts threads for each client. 
// In addition, it also traps SIGHUP signal as required.
// **********************************************************************
int main(int argc, char **argv){
    init_global_var();
    int retCd = check_parms(argc, argv);
    if (retCd != 0) {
        return retCd;
    }
    int mainPort = 0;
    if (argc == 3) {
        mainPort = atoi(argv[2]);
    }
    int maxConn = atoi(argv[1]);

    int newSockfds[10];
    //pthread_t tids[10], pubTId;
    pthread_t tids[10];

    // Bind signals to handling functions
    signal(SIGHUP, show_stats);

    int sockfd = init_socket(mainPort);

    listen(sockfd, maxConn);
    fflush(stdout);

    // Accept clients
    unsigned int sinSize = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    int i = 0;
    while (1) {

        newSockfds[i] = accept(sockfd, (struct sockaddr *)&clientAddr,
                &sinSize);
        pthread_create(&tids[i], NULL, handle_connection,
                (void *)&newSockfds[i]);
        statsData->connCli++;
        i++;
    }
    close(sockfd);
} // The main function creates a server that listens on an ephemeral port

// **********************************************************************
// Skip till the required token. 
// **********************************************************************
int skip_till_token(char *str, int tokenNum, int *placePtr) {
    int pointer = *placePtr, tokenCount = 1, foundToken = 0;
    while (pointer < strlen(str)) {
        if (tokenCount == tokenNum) {
            break;
        }
        if ((str[pointer] >= 'a' && str[pointer] <= 'z')
                || (str[pointer] >= 'A' && str[pointer] <= 'Z')
                || (str[pointer] >= '0' && str[pointer] <= '9')
                || str[pointer] == ':' || str[pointer] == '_'
                || str[pointer] == '-' || str[pointer] == '@') {
            pointer++;
            foundToken = 1;
            continue;
        }
        if (str[pointer] == ' ' || str[pointer] == '\t') {
            while (str[pointer] == ' ' || str[pointer] == '\t') {
                pointer++;
                if (pointer > strlen(str)) {
                    break;
                }
            }
            if (foundToken == 1) {
                tokenCount++;
                foundToken = 0;
            }
            pointer++;
            continue;
        }
        if (str[pointer] == '"') {
            pointer++;
            while (str[pointer] != '"') {
                pointer++;
                if (pointer > strlen(str)) {
                    break;
                }
            }
            foundToken = 0;
            tokenCount++;
        }
    }
    pointer--;
    if ((pointer > strlen(str)) || (tokenCount != tokenNum)) {
        return -1;
    }
    *placePtr = pointer;
    return 0;
}
    
// **********************************************************************
// find relevant token from the command. input is command recieved and 
// token number. The word associated with that token number is returned
// back in outToken
// **********************************************************************
int get_token(char *str, int tokenNum, char *outToken) {
    int pointer = 0, index = 0;
    if ((str[pointer] >= 'a' && str[pointer] <= 'z') 
            || (str[pointer] >= 'A' && str[pointer] <= 'Z')) {
        // valid 
    } else {
        return -1;
    }
    if (skip_till_token(str, tokenNum, &pointer) != 0) {
        return -1;
    }
    while (pointer < strlen(str)) {
        if ((str[pointer] >= 'a' && str[pointer] <= 'z')
                || (str[pointer] >= 'A' && str[pointer] <= 'Z')
                || (str[pointer] >= '0' && str[pointer] <= '9')
                || (str[pointer] == '-') || (str[pointer] == '_')) {
            outToken[index] = str[pointer];
            index++;
            pointer++;
            continue;
        }
        if (str[pointer] == '"') {
            outToken[index] = str[pointer];
            index++;
            pointer++;
            while (str[pointer] != '"') {
                outToken[index] = str[pointer];
                index++;
                pointer++;
                if (pointer > strlen(str)) {
                    break;
                }
            }
        }
        outToken[index] = '\0';
        break;// not a vlid char for message type
    }
    if (index == 0) {
        return -1;
    }
    if (pointer <= strlen(str)) {
        return 0;
    } else {
        return 1;
    }
}

// **********************************************************************
// find message type and validate the command sent
// **********************************************************************
int find_msg_type(char *command, char *messageType) {
    int pointer = 0;
    for (; pointer < strlen(command); pointer++) {
        if (command[pointer] >= 'a' && command[pointer] <= 'z') {
            break;
        }
        if (command[pointer] >= 'A' && command[pointer] <= 'Z') {
            break;
        }
    }
    int index = 0;
    while (1) {
        if (command[pointer] >= 'a' && command[pointer] <= 'z') {
            messageType[index] = command[pointer];
            index++;
            pointer++;
            continue;
        }
        if (command[pointer] >= 'A' && command[pointer] <= 'Z') {
            messageType[index] = command[pointer];
            index++;
            pointer++;
            continue;
        }
        break;// not a vlid char for message type
    }
    messageType[index] = '\0';
    if (strcmp(messageType, "unsub") == 0) {
        return 0;
    }
    if (strcmp(messageType, "pub") == 0) {
        return 0;
    }
    if (strcmp(messageType, "sub") == 0) {
        return 0;
    }
    if (strcmp(messageType, "name") == 0) {
        return 0;
    }
    return 1;
}

// **********************************************************************
// Send message to relevant client
// **********************************************************************
int send_msg(int sockfd, char *msg) {
    if (strlen(msg) == 0) {
        return 1;
    }
    int retCd = send(sockfd, msg, strlen(msg), 0);
    if (retCd <= 0) {
        return -1;
    }
    char retStr[128] = "\n";
    retCd = send(sockfd, retStr, strlen(retStr), 0);
    if (retCd <= 0) {
        return -1;
    }
    return 0;
}

// **********************************************************************
// Process command received. If received command is not valid, send an 
// invalid response
// **********************************************************************
void handle_command(int sockfd, char *command) {
    char msgType[128];
    if (strlen(command) <= 0) {
        send_msg(sockfd, ":invalid");
        return;
    }
    int err = find_msg_type(command, msgType);
    if (err == 1) { // invalid message type
        send_msg(sockfd, ":invalid");
        return;
    }
    char *namePtr;
    namePtr = strstr(command, msgType);
    namePtr += strlen(msgType);
    if (strcmp(msgType, "sub") == 0) {
        process_sub(sockfd, command);
    } else if (strcmp(msgType, "unsub") == 0) {
        process_unsub(sockfd, command);
    } else if (strcmp(msgType, "name") == 0) {
        process_name(sockfd, command);
    } else if (strcmp(msgType, "pub") == 0) {
        process_pub(sockfd, command);
    }
} // This function does plenty of things

// **********************************************************************
// Process name command. If this name is not already there, add it to the 
// client tree. Else ignore this name and send an invalid response
// **********************************************************************
void process_name(int sockfd, char *command) {
    char retStr[1024];
    memset(retStr, '\0', 1023);
    int retCd = get_token(command, 3, retStr);
    if (retCd == 0) { // 3rd argument present. invlaid
        send_msg(sockfd, ":invalid");
        return;
    }
    retCd = get_token(command, 2, retStr);
    if (retCd == 1 || valid_name(retStr) == 1) {
        send_msg(sockfd, ":invalid");
        return;
    }
    ClientData *clientData;
    StringMap *msgRoot;
    clientData = malloc(sizeof(ClientData));
    msgRoot = stringmap_init();
    clientData->msgRoot = msgRoot;
    clientData->sockfd = sockfd;
    int err = stringmap_add(clientRoot, retStr, clientData);
    if (err == 0) {
        send_msg(sockfd, ":invalid");
        //close(sockfd);
        //pthread_exit(0);
        return;
    }

    //print_names_only();
}

// **********************************************************************
// Search through client tree and find clint name for the received
// socket id
// **********************************************************************
int get_client_name(int sockfd, char *clientName) {
    ClientData *clientData;
    StringMap *currNode;
    currNode = NULL;
    currNode = stringmap_iterate(clientRoot, currNode);
    while (currNode != NULL){
        clientData = (ClientData*) currNode->item;
        if (sockfd == clientData->sockfd){
            strcpy(clientName,currNode->key);
            return 0;
        }
        currNode = stringmap_iterate(clientRoot,currNode);
    }
    strcpy(clientName, "");
    return -1;
}

// **********************************************************************
// Process unsub message from client. It searches topic tree and adds 
// client to the topic tree. It will not send data in case 
// name command was not received till this point
// **********************************************************************
void process_sub(int sockfd, char *command) {
    char retStr[1024], cliName[30], topic[30], *item;
    memset(retStr, '\0', 1023);
    int retCd = get_token(command, 3, retStr);
    if (retCd == 0) { // 3rd argument present. invlaid
        send_msg(sockfd, ":invalid");
        return;
    }
    retCd = get_token(command, 2, retStr);
    if (retCd == 1 || valid_name(retStr) == 1) {
        send_msg(sockfd, ":invalid");
        return;
    }
    strcpy(topic, retStr);

    retCd = get_client_name(sockfd, cliName);
    if (retCd == -1) { // if we have not received name
                  // earlier, then ignore command
        return;
    }
    StringMap *subCliRoot;
    subCliRoot = stringmap_search(topicRoot, topic);
    if (stringmap_search(subCliRoot, cliName) == 0) {
        //client not present for this topic
        statsData->subCount++;
    }
    item = malloc(strlen(cliName) + 2);
    strcpy(item, cliName);
    if (subCliRoot == NULL){
        subCliRoot = stringmap_init();
        stringmap_add(subCliRoot, cliName, item);
        stringmap_add(topicRoot, retStr, subCliRoot);
    } else {
        stringmap_add(subCliRoot, cliName, item);
    }
    //print_topic_tree();
}

// **********************************************************************
// Process unsub message from client. It searches topic tree and removes 
// client associated with this topici. It will not send data in case 
// name command was not received till this point
// **********************************************************************
void process_unsub(int sockfd, char *command) {
    char retStr[1024], cliName[30];
    memset(retStr, '\0', 1023);
    int retCd = get_token(command, 3, retStr);
    if (retCd == 0) { // 3rd argument present. invlaid
        send_msg(sockfd, ":invalid");
        return;
    }
    retCd = get_token(command, 2, retStr);
    if (retCd == 1 || valid_name(retStr) == 1) {
        send_msg(sockfd, ":invalid");
        return;
    }
    StringMap *subCliRoot;
    retCd = get_client_name(sockfd, cliName);
    if (retCd == -1) { // if we have not received name
                  // earlier, then ignore command
        return;
    }
    subCliRoot = stringmap_search(topicRoot, retStr);
    // if we get null, it means unsub was issued
    // before sub. So, nothing needs to be done.
    if (subCliRoot != NULL) {
        if (stringmap_search(subCliRoot, cliName) != 0) {
            statsData->unsubCount++;
        }
        stringmap_remove(subCliRoot, cliName);
    }
    //print_topic_tree();
}

// **********************************************************************
// Process pub message from client. It searches topic tree and finds 
// clients that need to receive messages. It then sends messages to 
// all these clients. It will not send data in case name command was 
// not received till this point
// **********************************************************************
void process_pub(int sockfd, char *command) {
    char retStr[1024], topic[30], toSendCli[30];
    StringMap *currNode, *subCliRoot;
    ClientData *clientData;
    char *remMsg, *msgSt, cliName[30];
    memset(retStr, '\0', 1023);
    int retCd = get_token(command, 2, retStr);
    if (retCd == 1 || valid_name(retStr) == 1) {
        send_msg(sockfd, ":invalid");
        return;
    }
    strcpy(topic, retStr);
    memset(retStr, '\0', 1023);
    retCd = get_token(command, 3, retStr);
    if (retCd == 0) {
    } else {
        send_msg(sockfd, ":invalid");
        return;
    }
    msgSt = strstr(command, retStr);
    remMsg = malloc(strlen(msgSt) + 2);
    strcpy(remMsg, msgSt);
    retCd = get_client_name(sockfd, cliName);
    if (retCd == -1) { // if we have not received name
                  // earlier, then ignore command
        return;
    }
    statsData->pubCount++;
    get_client_name(sockfd, cliName);
    subCliRoot = (StringMap*) stringmap_search(topicRoot, topic);
    currNode = NULL;
    currNode = stringmap_iterate(subCliRoot, currNode);
    while (currNode != NULL){
        strcpy(toSendCli, currNode->key);
        clientData = stringmap_search(clientRoot,toSendCli);
        if (clientData == NULL){
            continue;
        }
        form_and_send_msg(clientData->sockfd, cliName, topic, remMsg);
        currNode = stringmap_iterate(subCliRoot,currNode);
    }
    free(remMsg);
}

// **********************************************************************
// Used for debugging. Content of topic string map is printed. Sub
// structures with in item is also printed. item has a string map and 
// contains a nested string map that stores clients subscribed to 
// this topic
// **********************************************************************
void print_topic_tree(){
    StringMap *currNode, *cliNode, *subCliRoot;
    printf("************************** topics list start ***********\n");
    currNode =NULL;
    do {
        currNode = stringmap_iterate(topicRoot,currNode);
        if (currNode != NULL){
            printf("topic=%s=\n",currNode->key);
            subCliRoot = (StringMap*) currNode->item;
            cliNode = NULL;
            do {
                cliNode = stringmap_iterate(subCliRoot,cliNode);
                if (cliNode != NULL){
                    printf("\t\tclient=%s=\n",cliNode->key);
                }
            } while (cliNode != NULL);
        }
    } while (currNode != NULL);
    printf("************************** topics list end    ***********\n");
}

// **********************************************************************
// Used for debugging. Content of client string map is printed. Sub
// structures with in item are not printed. item has a structre and 
// one of the elements in the structure contains a nested string map 
// that can store messages to be sent to a client. This structure is not
// being used currently.
// **********************************************************************
void print_names_only(){
    StringMap *currNode;
    printf("************************** clients list start ***********\n");
    currNode = NULL;
    do {
        currNode = stringmap_iterate(clientRoot,currNode);
        if (currNode != NULL){
            printf("client =%s=\n",currNode->key);
        }
    } while (currNode != NULL);
    printf("************************** clients list end    ***********\n");

}

// **********************************************************************
// Used for debugging. Content of client string map is printed recursively
// **********************************************************************
void print_client_tree(){
    StringMap *currNode, *msgNode;
    ClientData *cliData;
    MsgData *msgData;
    printf("************************** clients tree start ***********\n");
    currNode = NULL;
    currNode = stringmap_iterate(clientRoot, currNode);
    while (currNode != NULL){
        cliData = (ClientData*) currNode->item;
        printf("sockfd=%d client name =%s=\n",cliData->sockfd,currNode->key);
        msgNode = NULL;
        msgNode = stringmap_iterate(cliData->msgRoot,msgNode);
       while (msgNode != NULL){
            msgData = (MsgData*)msgNode->item;
            printf("\t\tmsgID=%s, sentBy=%s, top=%s, msg=%s=\n",msgNode->key
                    ,msgData->msgSentBy,msgData->topic,msgData->msg);
            msgNode = stringmap_iterate(cliData->msgRoot,msgNode);
        }
        currNode = stringmap_iterate(clientRoot,currNode);
    }
    printf("************************** clients list end    ***********\n");
}

// **********************************************************************
// Init a socket and return back socket. If port is specified, use it.
// **********************************************************************
int init_socket(int port) {
    int err = 0;
    struct sockaddr_in hostAddr;
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(hostAddr.sin_zero), '\0', 8);
    hostAddr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd <= 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    err = bind(sockfd, (struct sockaddr *)&hostAddr, sizeof(struct sockaddr));
    if (err != 0) {
        fprintf(stderr, "psserver: unable to open socket for listening\n");
        fflush(stderr);
        exit(2);
    }
    struct sockaddr_in ad;

    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    err = getsockname(sockfd, (struct sockaddr*)&ad, &len);
    if (err != 0) {
        fprintf(stderr, "psserver: unable to get port number\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));
    fflush(stderr);

    return sockfd;
} // This function creates socket and binds it to port

// **********************************************************************
// Form the message to be sent and initiate sending message.
// **********************************************************************
void form_and_send_msg(int sockfd, char *sentBy, char *topic, char *msg){
    char *formatedMsg;
    formatedMsg = malloc(strlen(sentBy)
            + strlen(topic)
            + strlen(msg)
            + 5); //for :'s and other chars
    sprintf(formatedMsg, "%s:%s:%s", sentBy
            , topic, msg);
    send_msg(sockfd, formatedMsg);
    free(formatedMsg);
}

// **********************************************************************
// Once client disconnects, all data related to client is removed from 
// all topic related Data Structures
// **********************************************************************
void remove_client_from_topic_tree(char *topic, char *cliName){
    StringMap *currNode, *subCliRoot;
    char *item;
    currNode =NULL;
    currNode = stringmap_iterate(topicRoot, currNode);
    while (currNode != NULL){
        subCliRoot = (StringMap*) currNode->item;
        item = (char*) stringmap_search(subCliRoot, cliName);
        if (item != NULL){
            free(item);
            stringmap_remove(subCliRoot, cliName);
        }
        currNode = stringmap_iterate(topicRoot,currNode);
    }
}

// **********************************************************************
// Once client disconnects, all data related to client is removed from 
// all client related Data Structures
// **********************************************************************
void remove_client_from_client_tree(char *cliName){
    ClientData *item;
    item = (ClientData*) stringmap_search(clientRoot, cliName);
    if (item != NULL){
        stringmap_free(item->msgRoot);
        //free(item->msgRoot);
        //free(item);
        stringmap_remove(clientRoot, cliName);
    }
}

// **********************************************************************
// Once client disconnects, all data related to client is removed from 
// all Data Structures
// **********************************************************************
void remove_client_from_ds(char *cliName){
    StringMap *currNode;
    //print_topic_tree();
    currNode =NULL;
    currNode = stringmap_iterate(topicRoot, currNode);
    while (currNode != NULL){
        remove_client_from_topic_tree(currNode->key, cliName);
        currNode = stringmap_iterate(topicRoot,currNode);
    }
    //print_topic_tree();
    //print_client_tree();
    remove_client_from_client_tree(cliName);
    //print_client_tree();
}

// **********************************************************************
// After establishing connection, a thread is initiated and this funtion
// is called. This function keeps receiving data from that particular
// client and keeps processing each command.
// **********************************************************************
void *handle_connection(void *socketPtr) {
    int sockfd = *((int *)(socketPtr));
    char request[512], *buffer, cliName[30];
    int length, lastLength = 0;

    do {
        length = recv(sockfd, &request, 512, 0);
        request[length] = '\0';
        if (length != lastLength) {
            int k = 0, m;
            for (m = 0; m < strlen(request); m++) {
                if (request[m] == '\n') {
                    buffer = (char *)malloc(64);
                    strncpy(buffer, request + k, m - k);
                    buffer[m - k] = '\0';
                    k = m + 1;
                    handle_command(sockfd, buffer);
                    free(buffer);
                }
            }
            if (k < m) {
                handle_command(sockfd, request + k);
            }
            lastLength = length;
        }
    } while (length != 0);

    statsData->disconnCli++;
    //remove disconnected client form clientsRoot
    get_client_name(sockfd, cliName);
    remove_client_from_ds(cliName);
    pthread_exit(0);
} // This function handles the requests from clients


