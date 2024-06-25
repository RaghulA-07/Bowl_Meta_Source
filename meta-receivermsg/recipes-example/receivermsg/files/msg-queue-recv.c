#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

struct message
{
    long message_type;
    char message_text[100];
    msgqnum_t msg_qnum;
};
key_t key;

    int msgid;

    struct message msg;


void createMsgQeue() {
    printf("\n RECEIVER_HOOK: Inside createMsgQeue");
    // key = ftok("receiver2", 10);
    key = 1234;

    // Create message queue and return id

    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1)
    {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

     // number of messages currently on the message queue
    printf("\n RECEIVER_HOOK: Created msg queue");
}

int main()
{
    // createMsgQeue();

    key = 1234;

    // Create message queue and return id

    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1)
    {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }


    while (1)
    {
        memset(&msg.message_text, 0, sizeof(msg.message_text));

    // printf("\n");
    //     for(int i=0; i<sizeof(msg.message_text); i++) {
    //         printf(" %d", msg.message_text[i]);
    //     }

        if (msgrcv(msgid, &msg, sizeof(msg), 1, IPC_NOWAIT) == -1)
        {
            if (errno == ENOMSG) {
                // printf("\n RECEIVER_HOOK: IPC No message available");
            }
            else {
                printf("\n RECEIVER_HOOK:IPC Error\n");
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
            
        } else {
            printf("\n RECEIVER_HOOK: Received Data from receiver 1: %s", msg.message_text);
            // if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            //     perror("msgctl");
            //     exit(1);
            // }
            // printf("\n RECEIVER_HOOK: Removed Message queue");
            // createMsgQeue();
        }
        //  printf("\n RECEIVER_HOOK: while loop");
    }
    return 0;
}
