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

#define BUFFER_SIZE 32768
#define BACKLOG_SIZE 16

void createNetworkSocket(char* hostname, char* port);

void createNetworkSocket(char* hostname, char* port, struct addrinfo *addrs, struct addrinfo *hints) {
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port, hints, &addrs);
    int main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    int enable = 1;
    setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);
    listen(main_socket, BACKLOG_SIZE);
    fprintf(stdout, "Server listening on %s:%s\n", hostname, port);
}

int main(int argc, char** argv) {
    char* hostname;
    char* port; 
    if (argc == 1) {
        fprintf(stderr, "Error: 0 commandline arguments given, 1 required\n");
    } else if (argc == 2){
        hostname = argv[1];
        port = (char*)"80";
    } else if (argc == 3){
        hostname = argv[1];
        port = argv[2];
    } else {
        fprintf(stderr, "Error: %d commandline arguments given, 1 required\n", argc);
    }
    if (hostname && port) {
        struct addrinfo *addrs, hints = {};
        createNetworkSocket(hostname, port, addrs, &hints);
    }
    return 0;
}
