#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define SERVER_KEY 1234
#define CLIENT_KEY 5678

typedef struct {
    long mtype;
    char data[256];
} Message;

int main() {
    int server_queue_id, client_queue_id;

    // Create client message queue
    if ((client_queue_id = msgget(CLIENT_KEY, IPC_CREAT | 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    // Connect to the server
    if ((server_queue_id = msgget(SERVER_KEY, 0)) == -1) {
        perror("msgget");
        exit(1);
    }

    Message connect_msg;
    connect_msg.mtype = 1;
    connect_msg.data[0] = '\0'; // No data for connection request

    if (msgsnd(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Konektuar ne server\n");

    while (1) {
        // Send a request to the server
        Message request;
        request.mtype = CLIENT_KEY;

        printf("Shtyp kerkesen: ");
        fgets(request.data, sizeof(request.data), stdin);

        // Remove newline character from the input
        size_t len = strlen(request.data);
        if (len > 0 && request.data[len - 1] == '\n') {
            request.data[len - 1] = '\0';
        }

        // Send the request to the server
        if (msgsnd(client_queue_id, &request, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    }

    // Disconnect from the server
    Message disconnect_msg;
    disconnect_msg.mtype = 1;
    disconnect_msg.data[0] = '\0'; // No data for disconnect request

    if (msgsnd(server_queue_id, &disconnect_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
    }

    // Clean up client message queue
    msgctl(client_queue_id, IPC_RMID, NULL);

    printf("Ckycur nga serveri\n");

    return 0;
}

