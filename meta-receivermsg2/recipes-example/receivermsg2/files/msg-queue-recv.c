#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/ipc.h>

#include <sys/msg.h>
 
struct message {

    long message_type;

    char message_text[100];

};
 
int main() {

    key_t key;

    int msgid;

    struct message msg;
 
    // Generate unique key

    // key = ftok("receiver2", 10);
 
    key = 1234;
    // Create message queue and return id

    msgid = msgget(key, 0666 | IPC_CREAT);
    
    if(msgid == -1) {
    	perror("msgget failed");
    	exit(EXIT_FAILURE);
    }
 
    while(1) {

        // Receive message

        if(msgrcv(msgid, &msg, sizeof(msg), 1, 0) == -1) {
        	perror("msgrcv failed");
        	exit(EXIT_FAILURE);
        }
 
        printf("\nReceived Data from receiver 2: %s", msg.message_text);

    }
 
    printf("YOU ARE NOT IN WHILE LOOP\n");
    return 0;

}
