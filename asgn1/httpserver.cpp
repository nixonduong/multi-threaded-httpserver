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
// bool isValidResourceName(char* resourceName);

// bool isValidResourceName(char* resourceName) {
    
// }

void handlePutRequest(char* buffer, uint8_t sockfd) {
    char filename[27];
    char http[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t contentLength = 0;
    ssize_t responseVal = 0;
    uint16_t status = 200;
    uint8_t statusMSG = 0;
    ssize_t fileDescriptor;
    char requestData[BUFFER_SIZE];
    char* subString;
    char charLength[BUFFER_SIZE];
    sscanf(buffer, "%*s /%s %s", filename, http);
    subString = strstr(buffer, "Content-Length");
    sscanf(subString, "%*s %s", charLength);
    contentLength = atoi(charLength);
    subString = strstr(buffer, "\r\n\r\n");
    sscanf(subString, "\n %s", requestData);


    // printf("%s\n", "=================================");
    // printf("Filename: %s\n", filename);
    // printf("Http: %s\n", http);
    // printf("Content-Length: %d\n", contentLength);
    // printf("Request Data: %s\n", requestData);
    // printf("%s\n", "=================================");
    
    ssize_t fileTest = open(filename, O_RDONLY);
    if (fileTest == -1) {
        statusMSG = 1;
    }
    close(fileTest);

    fileDescriptor = open(filename, O_CREAT);
    if (fileDescriptor == -1) {
        status = 201;
    } else {
        write(fileDescriptor, requestData, contentLength);
    }
    close(fileDescriptor);
    if (statusMSG == 0) {
        responseVal = sprintf(response, "%s %d OK\r\n\r\n", http, status);
    }
    if (statusMSG == 1) {
        responseVal = sprintf(response, "%s %d CREATED\r\n\r\n", http, status);
    }

    // printf("Response: %s\n", response);
    // printf("ResponseVal: %d\n", responseVal);
  

    send(sockfd, response, responseVal, 0);
    close(sockfd);
}

void handleGetRequest(char* buffer, uint8_t sockfd) {
    ssize_t responseVal = 0;
    ssize_t contentLength = -1;
    uint16_t status = 200;
    char filename[27];
    char http[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char readBuffer[BUFFER_SIZE];
    char tempBuffer[BUFFER_SIZE];

    sscanf(buffer, "%*s /%s %s\r\n\r\n", filename, http);
    ssize_t fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        status = 404;
    } else {
        ssize_t bytesRead = 1;
        while (bytesRead) {
            bytesRead = read(fileDescriptor, tempBuffer, 1);
            contentLength += bytesRead;
            if (bytesRead == -1) {
                 break;
            }
        }
        close(fileDescriptor);
        responseVal = sprintf(response, "%s %d OK\r\nContent-Length: %zd\r\n\r\n", http, status, contentLength);
        send(sockfd, response, responseVal, 0);
    
        fileDescriptor = open(filename, O_RDONLY);
        bytesRead = 1;
        while (bytesRead) {
            bytesRead = read(fileDescriptor, readBuffer, 1);
            if (bytesRead == -1) {
                break;
            } else {
                write(sockfd, readBuffer, bytesRead);
            }
        }
        close(fileDescriptor);
    }
    close(sockfd);
}

void parseRequest(char* buffer, uint8_t sockfd) {
    char type[3];
    sscanf(buffer, "%s", type);
    if (strcmp(type, "GET") == 0) {
        handleGetRequest(buffer, sockfd);
    }
    if (strcmp(type, "PUT") == 0) {
        handlePutRequest(buffer, sockfd);
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
