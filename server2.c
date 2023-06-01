#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

#define SERVER_KEY 1234
#define CLIENT_KEY 5678
#define MAX_CLIENTS 10

typedef struct {
    long mtype;
    char data[256];
    pid_t pid;
} Message;

typedef struct {
    pthread_t thread;
    int connected;
    pid_t client_pid;
    int client_queue_id;  // Shto fushen client_queue_id per te ruajtur queue ID e klientit
} ClientInfo;

ClientInfo connected_clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_handler(void* arg) {
    int client_queue_id = *((int*)arg);
    Message request, response;
    ssize_t msg_size;

    pid_t client_pid = getpid();

    ClientInfo client_info;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_clients[i].thread == pthread_self()) {
            client_info = connected_clients[i];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    while ((msg_size = msgrcv(client_queue_id, &request, sizeof(Message) - sizeof(long), CLIENT_KEY, 0)) > 0) {
        printf("Mesazhi i pranuar nga klienti (PID %d): %s\n", client_info.client_pid, request.data);

        if (strcmp(request.data, "disconnect") == 0) {
            printf("Klienti (PID %d) eshte diskonektuar\n", client_info.client_pid);
            break;
        }

        response.mtype = CLIENT_KEY;
        strcpy(response.data, "Mesazhi i pranuar nga serveri");

        if (msgsnd(client_queue_id, &response, sizeof(Message) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            break;
        }
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_clients[i].thread == pthread_self()) {
            connected_clients[i].thread = 0;
            connected_clients[i].connected = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    msgctl(client_queue_id, IPC_RMID, NULL);  // Fshij message queue te klientit

    pthread_detach(pthread_self());
    return NULL;
}

int main() {
    int server_queue_id;

    if ((server_queue_id = msgget(SERVER_KEY, IPC_CREAT | 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Serveri eshte duke punuar\n");

    while (1) {
        Message connect_msg;
        if (msgrcv(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        if (strcmp(connect_msg.data, "disconnect") == 0) {
            printf("Klienti (PID %d) eshte diskonektuar\n", connect_msg.pid);
            continue;
        }

        int client_queue_id;
        if ((client_queue_id = msgget(CLIENT_KEY, IPC_CREAT | 0666)) == -1) {
            perror("msgget");
            exit(1);
        }

        pthread_mutex_lock(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!connected_clients[i].connected) {
                connected_clients[i].connected = 1;
                connected_clients[i].client_pid = connect_msg.pid;
                connected_clients[i].client_queue_id = client_queue_id;  // Ruaj queue ID te klientit
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        pthread_create(&connected_clients[i].thread, NULL, client_handler, &client_queue_id);
    }

    msgctl(server_queue_id, IPC_RMID, NULL);

    return 0;
}
