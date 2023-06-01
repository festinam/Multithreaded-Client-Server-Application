#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/types.h>

#define SERVER_KEY 1234
#define CLIENT_KEY 5678

typedef struct {
    long mtype;
    char data[256];
    pid_t pid;
} Message;

int main() {
    int server_queue_id, client_queue_id;

    if ((client_queue_id = msgget(CLIENT_KEY, IPC_CREAT | 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    if ((server_queue_id = msgget(SERVER_KEY, 0)) == -1) {
        perror("msgget");
        exit(1);
    }

    Message connect_msg;
    connect_msg.mtype = 1;
    connect_msg.data[0] = '\0';
    connect_msg.pid = getpid();

    if (msgsnd(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Konektuar me server\n");

    while (1) {
        Message request;
        request.mtype = CLIENT_KEY;
        request.pid = getpid();
        printf("Shkruaj kerkesen/mesazhin: ");
        fgets(request.data, sizeof(request.data), stdin);

        size_t len = strlen(request.data);
        if (len > 0 && request.data[len - 1] == '\n') {
            request.data[len - 1] = '\0';
        }

        if (msgsnd(client_queue_id, &request, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        Message response;
        if (msgrcv(client_queue_id, &response, sizeof(Message) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Pergjigja nga serveri: %s\n", response.data);

        if (strcmp(request.data, "disconnect") == 0) {
            printf("Diskonektuar nga serveri\n");
            break;
        }
    }

    Message disconnect_msg;
    disconnect_msg.mtype = 1;
    strcpy(disconnect_msg.data, "disconnect");
    disconnect_msg.pid = getpid();

    if (msgsnd(server_queue_id, &disconnect_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    msgctl(client_queue_id, IPC_RMID, NULL);

    return 0;
}
