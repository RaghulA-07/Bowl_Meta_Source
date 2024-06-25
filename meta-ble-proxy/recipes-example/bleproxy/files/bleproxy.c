#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_KEY 0x1234

struct message {
    long type;
    char text[100];
};

void send_message(long type, const char *text) {
    int msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    struct message msg;
    msg.type = type;
    snprintf(msg.text, sizeof(msg.text), "%s", text);

    if (msgsnd(msgid, &msg, sizeof(msg.text), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    printf("Message sent: %s\n", text);
}

int main() {
    send_message(1, "123e4567-e89b-12d3-a456-426614174000");
    sleep(5); // Simulate some time for data collection
    send_message(2, "");

    return 0;
}
