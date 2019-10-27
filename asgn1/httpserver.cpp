/*
Student: Nixon Duong
CruzID: niduong
Professor: Ethan L Miller
Course: CSE 130
Programming Assignment: PA1
File: httpserver.cpp
Dependencies: None
Description: 
*/

#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

#define BUFFER_SIZE 32768
#define BACKLOG_SIZE 16

uint8_t createNetworkSocket(char* hostname, char* port);
void processSocketConenctions(uint8_t main_socket, struct addrinfo *addrs);
void parseRequest(char* buffer, ssize_t bytesRecv, uint8_t sockfd);
void handleGetRequest(char* buffer, uint8_t sockfd);
void handlePutRequest(char* buffer, uint8_t sockfd);

void handlePutRequest(char* buffer, uint8_t sockfd) {
    char* token;
    token = strtok(buffer, " ");
    printf("%s\n", "PUT");
}

void handleGetRequest(char* buffer, uint8_t sockfd) {
    char filename[27];
    sscanf(buffer, "%*s /%s", filename);
    printf("%s\n", filename);
}

void parseRequest(char* buffer, ssize_t bytesRecv, uint8_t sockfd) {
    char type[3];
    sscanf(buffer, "%s", type);
    if (strcmp(type, "GET")) {
        handleGetRequest(buffer, sockfd);
    }
    if (strcmp(type, "PUT")) {
        handleGetRequest(buffer, sockfd);
    }
}

void processSocketConenctions(uint8_t main_socket, struct addrinfo *addrs) {
    char buffer[BUFFER_SIZE];
    uint8_t sockfd = 1;
    while(1) {
        sockfd = accept(main_socket, NULL, NULL);
        ssize_t bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
        parseRequest(buffer, bytesRecv, sockfd);
        // resolve request
    }
}

uint8_t createNetworkSocket(char* hostname, char* port, struct addrinfo *addrs, struct addrinfo *hints) {
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port, hints, &addrs);
    uint8_t main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    uint8_t enable = 1;
    setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);
    listen(main_socket, BACKLOG_SIZE);
    return main_socket;
}

int main(int argc, char** argv) {
    char* hostname = nullptr;
    char* port = nullptr;
    if (argc == 1) {
        fprintf(stderr, "Error: 0 commandline arguments given, 1 required\n");
        exit(1);
    } else if (argc == 2){
        hostname = argv[1];
        port = (char*)"80";
    } else if (argc == 3){
        hostname = argv[1];
        port = argv[2];
    } else {
        fprintf(stderr, "Error: %d commandline arguments given, 1 required\n", argc);
        exit(1);
    }
    if (hostname && port) {
        struct addrinfo *addrs, hints = {};
        uint8_t main_socket = createNetworkSocket(hostname, port, addrs, &hints);
        if (main_socket) processSocketConenctions(main_socket, addrs);
    }
    return 0;
}