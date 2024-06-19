#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define MAX_THREADS 10

typedef struct Job {
    char jobID[10];
    char command[256];
    int RUNNING;
    pid_t pid;
} Job;

typedef struct Server {
    int Concurrency;
    int jobs_running;
    int jobs_queued;
    int State;
} Server;

Server server = {1, 0, 0, 1};

pthread_t thread_pool[MAX_THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Job job_queue[MAX_THREADS];
int front = 0, rear = 0;

void *controller_thread(void *arg);
void *worker_thread(void *arg);
void exec_command();
void create_job_id(int x, char *id);
void queue_pop_pid(pid_t pid);

void sigchld_handler(int signum) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        queue_pop_pid(pid);
        server.jobs_running--;
        server.jobs_queued--;
        if (server.jobs_queued > 0) {
            exec_command();
        }
    }
    signal(SIGCHLD, sigchld_handler);
}

void *worker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (front == rear) {
            pthread_cond_wait(&cond, &mutex);
        }
        Job job = job_queue[front];
        front = (front + 1) % MAX_THREADS;
        pthread_mutex_unlock(&mutex);

        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {"/bin/sh", "-c", job.command, NULL};
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            job.pid = pid;
            job.RUNNING = 1;
            server.jobs_running++;
        }
    }
    return NULL;
}

void *controller_thread(void *arg) {
    int new_socket = *((int *)arg);
    free(arg);
    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);

    if (strncmp(buffer, "issueJob:", 9) == 0) {
        Job new_job;
        snprintf(new_job.command, sizeof(new_job.command), "%s", buffer + 9);
        create_job_id(rear, new_job.jobID);
        new_job.RUNNING = 0;
        pthread_mutex_lock(&mutex);
        job_queue[rear] = new_job;
        rear = (rear + 1) % MAX_THREADS;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        server.jobs_queued++;
    } else if (strncmp(buffer, "setConcurrency:", 15) == 0) {
        int concurrency = atoi(buffer + 15);
        server.Concurrency = concurrency;
    } else if (strncmp(buffer, "stopJob:", 8) == 0) {
        // Stop job implementation
    } else if (strncmp(buffer, "pollJobs:", 9) == 0) {
        // Poll jobs implementation
    } else if (strncmp(buffer, "exit", 4) == 0) {
        printf("SERVER HAS TO CLOSE\n");
        server.State = 0;
    }

    close(new_socket);
    return NULL;
}

void exec_command() {
    for (int i = 0; i < server.Concurrency; i++) {
        pthread_cond_signal(&cond);
    }
}

void create_job_id(int x, char *id) {
    snprintf(id, 10, "job_%d", x);
}

void queue_pop_pid(pid_t pid) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (job_queue[i].pid == pid) {
            job_queue[i].RUNNING = 0;
            break;
        }
    }
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGCHLD, sigchld_handler);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pthread_t mainThread;
    pthread_create(&mainThread, NULL, worker_thread, NULL);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&thread_pool[i], NULL, worker_thread, NULL);
    }

    while (server.State) {
        printf("Server Open and wait\n");
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        pthread_t controller;
        pthread_create(&controller, NULL, controller_thread, (void *)new_sock);
        printf("server:%d\n",server.State);
    }

    // for (int i = 0; i < MAX_THREADS; i++) {
    //     pthread_join(thread_pool[i], NULL);
    // }

    return 0;
}
