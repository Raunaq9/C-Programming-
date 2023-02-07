#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct GlbParms {
    FILE *serverRead;
    FILE *serverWrite;
    int argc;
    char **argv;
} GlbParms;

//the server socket
int serverSocket = 0;

//the client's socket
int mySocket = 0;

//socket address of the client
struct sockaddr_in myAddr;

//read and write of the server and the clients
FILE *serverRead, *serverWrite, *myRead;

// ****************************************************************
// Read data in wait mode from server
// ****************************************************************
int read_from_socket(FILE *readStream, char *buffer) {
    int err = 0;
    char myBuffer[100];
    do {
        if (fgets(myBuffer, 99, readStream) == NULL) {
            return 10;
        }
    } while (strlen(myBuffer) == 0);
    strcpy(buffer, myBuffer);
    buffer[strlen(myBuffer) - 1] = '\0';
    return err;
}

// ****************************************************************
// Read data in no wait mode from server
// ****************************************************************
int read_no_wait_from_socket(FILE *readStream, char *buffer) {
    char myBuffer[300];
    if (fgets(myBuffer, 299, readStream) == NULL) {
        return 10;
    }
    if (strlen(myBuffer) > 0) {
        strcpy(buffer, myBuffer);
        buffer[strlen(myBuffer) - 1] = '\0';
        return 1;
    }
    return 0;
}

// ****************************************************************
// Write data to server
// ****************************************************************
void write_to_socket(FILE *writeStream, char *msg) {
    fprintf(writeStream, "%s\n", msg);
    fflush(writeStream);
    fflush(writeStream);
}

// ****************************************************************
// Read data in no wait mode from server. Ignore invalid messages and 
// 0 length messages.
// ****************************************************************
void get_msg_from_server(FILE *serverStream) {
    char *message = (char*)malloc(sizeof(char) * 1000);
    int err = read_from_socket(serverStream, message);
    if (err != 0) {
        return;
    }
}

// ****************************************************************
// Print messages for client
// ****************************************************************
void process_message(char *message){
    fprintf(stdout, "%s\n", message);
    fflush(stdout);
}

// ****************************************************************
// Process all inbound messages
// ****************************************************************
void *process_inward_messages(void *args) {
    char msg[300];
    GlbParms *parm = (GlbParms*)args;
    while (1) {
        int retCd = read_no_wait_from_socket(parm->serverRead, msg);
        if (strlen(msg) > 0) {
            //printf("process_inward_messages: retCd=%d\n",retCd);
        }
        if (retCd == 1) {
            process_message(msg);
        } else if (retCd > 1) {
            //fprintf(stderr, "psclient: server connection terminated\n");
            //fflush(stderr);
            //exit(4);
            //break;
        }
        usleep(100);
    }
    return NULL;
}

// ****************************************************************
// Process interactive chat with server. repeatedly get input and 
// send to client. start another thread to process inbound messages 
// from server.
// ****************************************************************
void process_chat(FILE *inServerRead, FILE *inServerWrite,
        int inArgc, char **inArgv){
    //char myBuffer[100], message[256];
    char myBuffer[100];
    char *retStr;
    pthread_t myID;
    GlbParms *parms;
    parms = (GlbParms*) malloc(sizeof(GlbParms));
    parms->serverRead = serverRead;
    parms->serverWrite = serverWrite;
    parms->argc = inArgc;
    parms->argv = inArgv;
    pthread_create(&myID, NULL, &process_inward_messages, (void*)parms);
    sprintf(myBuffer, "name %s", inArgv[2]);
    write_to_socket(serverWrite, myBuffer);
    for (int i = 3; i < inArgc; i++) {
        sprintf(myBuffer, "sub %s", inArgv[i]);
        write_to_socket(serverWrite, myBuffer);
    }
    while (1) {
        char msg[1024];
        retStr = fgets(msg, 1023, stdin);
        if (retStr == NULL) {
            break;
        }
        retStr = strchr(msg, '\n');
        if (retStr != NULL) {
            *retStr = '\0';
        }
        write_to_socket(serverWrite, msg);
    }

}

// ****************************************************************
// Validate names and other tokens to ensure that there are no invalid
// chars in them
// ****************************************************************
int valid_name(char *str){
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

// ****************************************************************
// Validate token to ensure that only numeric vlaues are present
// ****************************************************************
int is_numeric(char *str){
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 1;
        }
    }
    return 0;
}

// ****************************************************************
// Validate all arguments passed to program
// ****************************************************************
int check_parms(int argc, char **argv){
    if (argc < 3) {
        fprintf(stderr, "Usage: psclient portnum name [topic] ...\n");
        fflush(stderr);
        return 1;
    }
    if (valid_name(argv[2])) {
        fprintf(stderr, "psclient: invalid name\n");
        fflush(stderr);
        return 2;
    }
    for (int i = 3; i < argc; i++) {
        if (valid_name(argv[i])) {
            fprintf(stderr, "psclient: invalid topic\n");
            fflush(stderr);
            return 2;
        }
    }

    if (is_numeric(argv[1])) {
        fprintf(stderr, "psclient: unable to connect to port %s\n",argv[1]);
        fflush(stderr);
        return 3;
    }
    int mainPort = atoi(argv[1]);
    if (mainPort <= 0) {
        fprintf(stderr, "psclient: unable to connect to port %d\n",mainPort);
        fflush(stderr);
        return 3;
    }
    return 0;
}

// ****************************************************************
// Main function. Initiates socket connections and calls function 
// to process chat
// ****************************************************************
int main(int argc, char **argv){
    int retCd = check_parms(argc, argv);
    if (retCd != 0) {
        return retCd;
    }
    int mainPort = atoi(argv[1]);

    struct sockaddr_in sAddr;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sAddr.sin_port = htons(mainPort);
    int err = connect(serverSocket, (struct sockaddr *)&sAddr, sizeof(sAddr));
    if (err == -1) {
        fprintf(stderr, "psclient: unable to connect to port %d\n",mainPort);
        fflush(stderr);
        return 3;
    }
    int socketWrite = dup(serverSocket);
    serverRead = fdopen(serverSocket, "r");
    serverWrite = fdopen(socketWrite, "w");
    process_chat(serverRead, serverWrite, argc, argv);
    fclose(serverRead);
    fclose(serverWrite);
    close(serverSocket);
    close(socketWrite);
    close(mySocket);
    return 0;
}

