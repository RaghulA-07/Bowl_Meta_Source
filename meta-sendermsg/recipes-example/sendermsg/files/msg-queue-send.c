#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct message
{

    long message_type;

    char message_text[100];
};

int main()
{

    key_t key;

    int msgid;

    struct message msg;

    // Generate unique key

    // key = ftok("progfile", 65);
    key = 1234;

    msgid = msgget(key, 0666 | IPC_CREAT);

    printf("\nsender msgid: %ls", &msgid);

    msg.message_type = 1;

int n=0;
char c[100];

    while (1)
    {
        sleep(3);

        if(n == 99) {
            n = 0;
        }

        n++;
        c[n] = n;

        sprintf(c, "%d", n);
        
        strcpy(msg.message_text, c);

        msgsnd(msgid, &msg, sizeof(msg), 0);

        printf("\nData sent: %s", msg.message_text);

        msg.message_type = 1;
    }

    return 0;
}

