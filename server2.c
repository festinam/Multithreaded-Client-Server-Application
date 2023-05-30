#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>

#define SERVER_KEY 1234
#define CLIENT_KEY 5678
#define MAX_CLIENTS 10

typedef struct {
    long mtype;
    char data[256];
} Message;

typedef struct {
    int client_queue_id;
    pthread_t thread;
} ClientInfo;

ClientInfo connected_clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_handler(void* arg) {
    int client_queue_id = *((int*)arg);
    Message request, response;
    ssize_t msg_size;

    // Get the client's PID and IPC ID
    pid_t client_pid = getpid();
    int client_ipc_id = client_queue_id;

    // Display client connection information
    printf("Klienti i ri i konektuar - PID: %d, IPC ID: %d\n", client_pid, client_ipc_id);

    while ((msg_size = msgrcv(client_queue_id, &request, sizeof(Message) - sizeof(long), CLIENT_KEY, 0)) > 0) {
        // Process the request
        printf("msgrcv: %s\n", request.data);
        printf("Mesazhi nga klienti (PID: %d, IPC ID: %d): %s\n", client_pid, client_ipc_id, request.data);

        // Check if the client wants to disconnect
        if (strcmp(request.data, "disconnect") == 0) {
            printf("Klienti u ckyc (PID: %d, IPC ID: %d)\n", client_pid, client_ipc_id);
            break;
        }

        // Generate the response
        response.mtype = CLIENT_KEY;
        strcpy(response.data, "Mesazhi nga serveri");

        // Send the response back to the client
        if (msgsnd(client_queue_id, &response, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            break;
        }
    }

    // Remove the client from the connected clients list
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_clients[i].client_queue_id == client_queue_id) {
            connected_clients[i].client_queue_id = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // Display client disconnection information
    printf("Klienti u ckyc (PID: %d, IPC ID: %d)\n", client_pid, client_ipc_id);

    // Terminate the thread
    pthread_detach(pthread_self());
    return NULL;
}




int main() {
    int server_queue_id;

    // Create server message queue
    if ((server_queue_id = msgget(SERVER_KEY, IPC_CREAT | 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Serveri eshte duke punuar\n");

    while (1) {
        // Accept connections from clients
        Message connect_msg;
        if (msgrcv(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        // Check if the client wants to disconnect
        if (strcmp(connect_msg.data, "disconnect") == 0) {
            printf("Klienti u diskonektua\n");
            continue;
        }

        // Create a new client message queue
        int client_queue_id;
        if ((client_queue_id = msgget(CLIENT_KEY + rand() % 100, IPC_CREAT | 0666)) == -1) {
            perror("msgget");
            exit(1);
        }

        // Add the client to the connected clients list
        pthread_mutex_lock(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (connected_clients[i].client_queue_id == 0) {
                connected_clients[i].client_queue_id = client_queue_id;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Spawn a new thread to handle the client
        pthread_create(&connected_clients[i].thread, NULL, client_handler, &connected_clients[i].client_queue_id);
    }

    // Clean up the server message queue
    msgctl(server_queue_id, IPC_RMID, NULL);

    return 0;
}

