/*
Student: Nixon Duong
CruzID: niduong
Professor: Ethan L Miller
Course: CSE 130
Programming Assignment: PA3
File: httpserver.cpp
Dependencies: None
Description: Multi-threaded HTTP server that supports
             Alias
*/

#include <err.h>
#include <fcntl.h>
#include <functional>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUFFER_SIZE 16384
#define BACKLOG_SIZE 16

bool isValidResourceName(char resourceName[]);
bool createNewAlias(char *existing_name, char *new_name);
char *followAliasToObject(char *alias, char *type);

void handlePatchRequest(char *buffer, ssize_t sockfd);
void handlePutRequest(char *buffer, ssize_t sockfd);
void handleGetRequest(char *buffer, ssize_t sockfd);
void parseRequest(char *buffer, ssize_t sockfd);
void processSocketConnections(ssize_t main_socket);
ssize_t createNetworkSocket(char *hostname, char *port);
void *threadFunction(void *);

ssize_t mappingFd = -1;
std::hash<std::string> hashFunction;
ssize_t count = 0;
ssize_t in = 0;
ssize_t out = 0;
ssize_t sockfdItem = 0;
ssize_t sharedBuffer[BUFFER_SIZE];
pthread_mutex_t lock;
pthread_cond_t dataAvailable;
pthread_cond_t spaceAvailable;

/*
Checks if resource name is valid
@type {boolean}
*/
bool isValidResourceName(char *resourceName) {
  bool valid = true;
  if (resourceName == nullptr) {
    valid = false;
  } else {
    ssize_t length = strlen(resourceName);
    valid = true;
    if (length != 27) {
      return false;
    } else {
      for (ssize_t i = 0; i < length; i++) {
        ssize_t asciiVal = static_cast<ssize_t>(resourceName[i]);
        if ((asciiVal >= 48 && asciiVal <= 57) ||
            (asciiVal >= 97 && asciiVal <= 122) ||
            (asciiVal >= 65 && asciiVal <= 90) || (asciiVal == 95) ||
            (asciiVal == 45)) {
          valid = true;
        } else {
          return false;
        }
      }
    }
  }
  return valid;
}

/*
Server attempts to create a new alias
@type {bool}
*/
bool createNewAlias(char *existing_name, char *new_name) {
  char *obj = followAliasToObject(existing_name, (char *)"PATCH");
  bool status = true;
  if (obj == nullptr) {
    status = false;
  } else {
    if ((sizeof(existing_name) + sizeof(new_name)) > 128) {
      status = false;
    } else {
      uint32_t hashVal = hashFunction(new_name) % 8000;
      char entry[128];
      int16_t entrySize = sprintf(entry, "%s %s", new_name, existing_name);
      pwrite(mappingFd, entry, entrySize, hashVal);
    }
  }
  return (status);
}

/*
Server attempts to follow aliases until it reaches an actual object or until a
name doesn't exist
@type {char*}
*/
char *followAliasToObject(char *alias, char *type) {
  ssize_t fileTest = open(alias, O_RDONLY);
  char *obj = nullptr;
  if (fileTest == -1 && strcmp(type, "PUT") != 0) {
    char entry[128];
    char value[128];
    uint32_t hashVal = hashFunction(alias) % 8000;
    pread(mappingFd, entry, 128, hashVal);
    sscanf(entry, "%*s %s", value);
    obj = value;
  } else {
    // alias is already the filename
    obj = alias;
  }
  return obj;
}

/*
Parses Patch Request Header
@type {void}
*/
void handlePatchRequest(char *buffer, ssize_t sockfd) {
  char resource[27];
  char http[BUFFER_SIZE];
  char *subString;
  char charLength[BUFFER_SIZE];
  char body[BUFFER_SIZE];
  char existing_name[27];
  char new_name[27];
  ssize_t responseVal = 0;
  char response[BUFFER_SIZE];
  ssize_t contentLength = 0;
  char contentLengthHeader[] = "Content-Length: 0\r\n\r\n";
  sscanf(buffer, "%*s %s %s", resource, http);
  subString = strstr(buffer, "Content-Length");
  sscanf(subString, "%*s %s", charLength);
  contentLength = atoi(charLength);
  if (resource[0] == '/') {
    memmove(resource, resource + 1, strlen(resource));
  }
  read(sockfd, body, BUFFER_SIZE);
  sscanf(body, "%*s %s %s\r\n", existing_name, new_name);

  // Check if PATCH could return errors
  if (createNewAlias(existing_name, new_name)) {
    responseVal =
        sprintf(response, "%s %d OK\r\n%s", http, 200, contentLengthHeader);
  } else {
    responseVal = sprintf(response, "%s %d Bad Request\r\n%s", http, 404,
                          contentLengthHeader);
  }
  send(sockfd, response, responseVal, 0);
  close(sockfd);
}

/*
Parses Put Request Header for resource name and content length.
Reads content-length bytes of data and writes to specified
resource name
@type {void}
*/
void handlePutRequest(char *buffer, ssize_t sockfd) {
  char *filename;
  char filenameData[27];
  char http[BUFFER_SIZE];
  char response[BUFFER_SIZE];
  char contentLengthHeader[] = "Content-Length: 0\r\n\r\n";
  ssize_t contentLength = 0;
  ssize_t responseVal = 0;
  uint16_t status = 200;
  uint8_t statusMSG = 0;
  ssize_t fileDescriptor;
  char requestData[BUFFER_SIZE];
  char *subString;
  char charLength[BUFFER_SIZE];
  sscanf(buffer, "%*s %s %s", filenameData, http);
  if (filenameData[0] == '/') {
    memmove(filenameData, filenameData + 1, strlen(filenameData));
  }
  filename = followAliasToObject(filenameData, (char *)"PUT");
  if (isValidResourceName(filename)) {
    subString = strstr(buffer, "Content-Length");
    sscanf(subString, "%*s %s", charLength);
    contentLength = atoi(charLength);
    subString = strstr(buffer, "\r\n\r\n");
    ssize_t recvBytes = recv(sockfd, requestData, sizeof(requestData), 0);
    ssize_t fileTest = open(filename, O_RDONLY);
    if (fileTest == -1) {
      statusMSG = 1;
      status = 201;
    }
    close(fileTest);
    fileDescriptor =
        open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fileDescriptor == -1) {
      status = 201;
    } else {
      if (recvBytes >= contentLength) {
        write(fileDescriptor, requestData, contentLength);
      }
      if (recvBytes < contentLength) {
        write(fileDescriptor, requestData, recvBytes);
        contentLength -= recvBytes;
        ssize_t counter = 0;
        ssize_t bytesRead = 0;
        char readBuffer[BUFFER_SIZE];
        while (counter < contentLength) {
          bytesRead = recv(sockfd, readBuffer, 1, 0);
          if (bytesRead == -1) {
            break;
          } else {
            write(fileDescriptor, readBuffer, bytesRead);
          }
          counter++;
        }
      }
      if (contentLength == 0) {
        write(fileDescriptor, requestData, recvBytes);
        ssize_t bytesRead = 0;
        char readBuffer[BUFFER_SIZE];
        while (bytesRead) {
          bytesRead = recv(sockfd, readBuffer, 1, 0);
          if (bytesRead == -1) {
            break;
          } else {
            write(fileDescriptor, readBuffer, bytesRead);
          }
        }
      }
    }
    close(fileDescriptor);
    if (statusMSG == 0) {
      responseVal = sprintf(response, "%s %d OK\r\n%s", http, status,
                            contentLengthHeader);
    }
    if (statusMSG == 1) {
      responseVal = sprintf(response, "%s %d Created\r\n%s", http, status,
                            contentLengthHeader);
    }
    send(sockfd, response, responseVal, 0);
  } else {
    status = 400;
    responseVal = sprintf(response, "%s %d Bad Request\r\n%s", http, status,
                          contentLengthHeader);
    send(sockfd, response, responseVal, 0);
  }
  close(sockfd);
}

/*
Parses GET Request Header for resource name.
Sends data from specified resource name back to client if
resource name is valid
@type {void}
*/
void handleGetRequest(char *buffer, ssize_t sockfd) {
  ssize_t responseVal = 0;
  ssize_t contentLength = 0;
  uint16_t status = 200;
  char *filename;
  char filenameData[BUFFER_SIZE];
  char http[BUFFER_SIZE];
  char response[BUFFER_SIZE];
  char readBuffer[BUFFER_SIZE];
  char tempBuffer[BUFFER_SIZE];
  sscanf(buffer, "%*s %s %s\r\n\r\n", filenameData, http);
  if (filenameData[0] == '/') {
    memmove(filenameData, filenameData + 1, strlen(filenameData));
  }
  filename = followAliasToObject(filenameData, (char *)"GET");
  if (isValidResourceName(filename)) {
    ssize_t fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
      status = 404;
      responseVal =
          sprintf(response, "%s %d Not Found\r\nContent-Length: 0\r\n\r\n",
                  http, status);
      send(sockfd, response, responseVal, 0);
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
      responseVal = sprintf(response, "%s %d OK\r\nContent-Length: %zd\r\n\r\n",
                            http, status, contentLength);
      send(sockfd, response, responseVal, 0);
      fileDescriptor = open(filename, O_RDONLY);
      ssize_t counter = 0;
      while (counter < contentLength) {
        bytesRead = read(fileDescriptor, readBuffer, 1);
        if (bytesRead == -1) {
          break;
        } else {
          send(sockfd, readBuffer, bytesRead, 0);
        }
        counter++;
      }
      close(fileDescriptor);
    }
  } else {
    status = 400;
    responseVal =
        sprintf(response, "%s %d Bad Request\r\nContent-Length: 0\r\n\r\n",
                http, status);
    send(sockfd, response, responseVal, 0);
  }
  close(sockfd);
}

/*
Parses request for type and directs program on how it should handle
specific request type
@type {void}
*/
void parseRequest(char *buffer, ssize_t sockfd) {
  char type[BUFFER_SIZE];
  char http[BUFFER_SIZE];
  char filename[27];
  sscanf(buffer, "%s %s %s\r\n\r\n", type, filename, http);
  if (strcmp(type, "GET") == 0) {
    handleGetRequest(buffer, sockfd);
  } else if (strcmp(type, "PUT") == 0) {
    handlePutRequest(buffer, sockfd);
  } else if (strcmp(type, "PATCH") == 0) {
    handlePatchRequest(buffer, sockfd);
  } else {
    ssize_t responseVal = 0;
    char response[BUFFER_SIZE];
    responseVal =
        sprintf(response, "%s %d Internal Server Error\r\n\r\n", http, 500);
    send(sockfd, response, responseVal, 0);
    close(sockfd);
  }
}

/*
Accepts socket connection from backlog, received data and
passes data to parseRequest() for further parsing
@type {void}
*/
void processSocketConnections(ssize_t main_socket) {
  sockfdItem = -1;
  while (1) {
    sockfdItem = accept(main_socket, NULL, NULL);
    if (sockfdItem >= 0) {
      pthread_mutex_lock(&lock);
      while (count == BUFFER_SIZE) {
        pthread_cond_wait(&spaceAvailable, &lock);
      }
      sharedBuffer[in] = sockfdItem;
      count++;
      in = (in + 1) % BUFFER_SIZE;
      pthread_cond_signal(&dataAvailable);
      pthread_mutex_unlock(&lock);
    }
    sockfdItem = -1;
  }
}

/*
Creates a network socket and returns the socket file
descriptor
@type {ssize_t}
*/
ssize_t createNetworkSocket(char *hostname, char *port, struct addrinfo *addrs,
                            struct addrinfo *hints) {
  hints->ai_family = AF_INET;
  hints->ai_socktype = SOCK_STREAM;
  getaddrinfo(hostname, port, hints, &addrs);
  ssize_t main_socket =
      socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
  ssize_t enable = 1;
  setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);
  listen(main_socket, BACKLOG_SIZE);
  return main_socket;
}

void *threadFunction(void *) {
  while (1) {
    pthread_mutex_lock(&lock);
    while (count == 0) {
      pthread_cond_wait(&dataAvailable, &lock);
    }
    char buffer[BUFFER_SIZE];
    sockfdItem = sharedBuffer[out];
    count--;
    out = (out + 1) % BUFFER_SIZE;
    pthread_cond_signal(&spaceAvailable);
    pthread_mutex_unlock(&lock);
    recv(sockfdItem, buffer, sizeof(buffer), 0);
    parseRequest(buffer, sockfdItem);
  }
}

/*
Takes valid commandline arguments and initializes self
port and hostname
descriptor
@type {int}
*/
int main(int argc, char **argv) {
  ssize_t nThreads = 0;
  char *logFile = nullptr;
  char *hostname = nullptr;
  char *port = nullptr;
  char *mappingFile = nullptr;
  for (;;) {
    int8_t option = getopt(argc, argv, "N:l:a:");
    if (option == EOF)
      break;
    switch (option) {
    case 'N':
      nThreads = atoi(optarg);
      break;
    case 'l':
      logFile = optarg;
      break;
    case 'a':
      mappingFile = optarg;
      break;
    }
  }
  if (argc == optind) {
    fprintf(stderr, "Error: 0 commandline arguments given, 1 required\n");
    exit(1);
  } else if ((argc - optind) > 2) {
    fprintf(stderr, "Error: %d commandline arguments given, 1 required\n",
            argc - optind);
    exit(1);
  } else {
    hostname = argv[optind];
    if ((argc - 1) > optind) {
      port = argv[optind + 1];
    } else {
      port = (char *)"80";
    }
  }
  if (mappingFile == nullptr) {
    fprintf(stderr, "Error: no mapping file specified\n");
    exit(1);
  }
  if (nThreads == 0) {
    nThreads = 4;
  }
  mappingFd = open(mappingFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (hostname && port) {
    struct addrinfo *addrs = nullptr;
    struct addrinfo hints = {};
    pthread_t *threadPool = (pthread_t *)malloc(nThreads * sizeof(pthread_t));
    for (ssize_t i = 0; i < nThreads; i++) {
      pthread_create(&threadPool[i], NULL, threadFunction, NULL);
    }
    ssize_t main_socket = createNetworkSocket(hostname, port, addrs, &hints);
    if (main_socket)
      processSocketConnections(main_socket);
  }
  return 0;
}
