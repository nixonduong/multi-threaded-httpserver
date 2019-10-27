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
void processSocketConenctions(uint8_t main_socket);
void parseRequest(char* buffer, uint8_t sockfd);
void handleGetRequest(char* buffer, uint8_t sockfd);
void handlePutRequest(char* buffer, uint8_t sockfd);

void handlePutRequest(char* buffer, uint8_t sockfd) {

}

void handleGetRequest(char* buffer, uint8_t sockfd) {
    char filename[27];
    char http[BUFFER_SIZE];
    ssize_t responseVal;
    sscanf(buffer, "%*s /%s %s\r\n\r\n", filename, http);
    char response[BUFFER_SIZE];
    uint16_t status = 200;
    char readBuffer[BUFFER_SIZE];
    char responseData[BUFFER_SIZE];
    ssize_t contentLength = -1;
    ssize_t fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        status = 404;
    } else {
        ssize_t bytesRead = 1;
        while (bytesRead) {
            bytesRead = read(fileDescriptor, readBuffer, 1);
            contentLength += bytesRead;
            if (bytesRead == -1) {
                break;
            } else {
                strcat(responseData, readBuffer);
            }
        }
        // Figure out why response data has an endl
        close(fileDescriptor);
        responseVal = sprintf(response, "%s %d OK\r\nContent-Length: %zd\r\n\r\n%s", http, status, contentLength, responseData);
        send(sockfd, response, responseVal,0);
        close(sockfd);
    }
}

void parseRequest(char* buffer, uint8_t sockfd) {
    char type[3];
    sscanf(buffer, "%s", type);
    if (strcmp(type, "GET")) {
        handleGetRequest(buffer, sockfd);
    }
    if (strcmp(type, "PUT")) {
        handleGetRequest(buffer, sockfd);
    }
}

void processSocketConenctions(uint8_t main_socket) {
    char buffer[BUFFER_SIZE];
    uint8_t sockfd = 1;
    while(1) {
        sockfd = accept(main_socket, NULL, NULL);
        recv(sockfd, buffer, sizeof(buffer), 0);
        parseRequest(buffer, sockfd);
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
        struct addrinfo *addrs = nullptr;
        struct addrinfo hints = {};
        uint8_t main_socket = createNetworkSocket(hostname, port, addrs, &hints);
        if (main_socket) processSocketConenctions(main_socket);
    }
    return 0;
}