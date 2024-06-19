#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

void issueJob(int sock, char *job) {
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "issueJob:%s", job);
    send(sock, buffer, strlen(buffer), 0);
}

void setConcurrency(int sock, int N) {
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "setConcurrency:%d", N);
    send(sock, buffer, strlen(buffer), 0);
}

void stopJob(int sock, char *jobID) {
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "stopJob:%s", jobID);
    send(sock, buffer, strlen(buffer), 0);
}

void pollJobs(int sock, char *type) {
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "pollJobs:%s", type);
    send(sock, buffer, strlen(buffer), 0);
}

void exitServer(int sock) {
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer), "exit");
    send(sock, buffer, strlen(buffer), 0);
}

int main(int argc, char const *argv[]) {
    struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Παραδείγματα χρήσης των συναρτήσεων
    // issueJob(sock,"ls");
    // setConcurrency(sock, 5);
    // stopJob(sock, "job_1");
    // pollJobs(sock, "running");
    exitServer(sock);
    
    close(sock);
    return 0;
}
