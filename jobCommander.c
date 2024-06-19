#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


typedef struct {
    char server_name[100];
    int port;
    char command[256];
    char cmd[256];
} Server;


typedef struct {
    int command_type;
    char cmd[256];
} Command;

void serialize(Command *data, char *buffer) {
    memcpy(buffer, data, sizeof(Command));
}

int main(int argc, char const *argv[]) {
    Command command;
    Server server;
    server.port = 8080;  // Default port

    printf("number of args:%d\n",argc);
    
    if (argc > 1) {
        strcpy(server.server_name, argv[1]);
        printf("server_name = %s\n", server.server_name);

        if (argc > 2) {
            server.port = atoi(argv[2]);
            printf("server_port = %d\n", server.port);
        }

        if (argc > 3) {
            strcpy(server.command, argv[3]);
            printf("server_command = %s\n", server.command);
        }
        if (argc > 4) {
            // Concatenate all remaining arguments into server.cmd
            server.cmd[0] = '\0';  // Initialize the string to be empty
            for (int i = 4; i < argc; i++) {
                strcat(server.cmd, argv[i]);
                if (i < argc - 1) {
                    strcat(server.cmd, " ");
                }
            }
            printf("server_cmd = %s\n", server.cmd); 
        }
    } else {
        fprintf(stderr, "Usage: %s <server_name> <port> <command> <cmd>\n", argv[0]);
        return 1;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server.port);

    if (inet_pton(AF_INET, server.server_name, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    

    // Send the command to the server
    if (strcmp(server.command, "issueJob") == 0) {
        command.command_type = 1;
        strcpy(command.cmd,server.cmd);
        char buffer[1024];
        // Serialize the data
        serialize(&command, buffer);
        // Send the data
        send(sock, buffer, sizeof(Command), 0);
        printf("Data sent\n");
    } else if (strcmp(server.command, "setConcurrency") == 0) {
        char message[1024];
        snprintf(message, sizeof(message), "setConcurrency:%s", server.cmd);
        send(sock, message, strlen(message), 0);
        printf("Message sent: %s\n", message);
    } else if (strcmp(server.command, "stopJob") == 0) {
        char message[1024];
        snprintf(message, sizeof(message), "stopJob:%s", server.cmd);
        send(sock, message, strlen(message), 0);
        printf("Message sent: %s\n", message);
    } else if (strcmp(server.command, "pollJobs") == 0) {
        char message[1024];
        snprintf(message, sizeof(message), "pollJobs:%s", server.cmd);
        send(sock, message, strlen(message), 0);
        printf("Message sent: %s\n", message);
    } else if (strcmp(server.command, "exit") == 0) {
        send(sock, "exit", strlen("exit"), 0);
        printf("Message sent: exit\n");
    } else {
        printf("Invalid command\n");
        close(sock);
        return 1;
    }

    // Read the server's response
    int valread = read(sock, buffer, 1024);
    if (valread > 0) {
        printf("Server response: %s\n", buffer);
    } else {
        printf("No response from server\n");
    }

    close(sock);
    return 0;
}
