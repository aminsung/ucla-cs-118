#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, length, n;
    socklen_t recv_len;
    struct sockaddr_in sender, receiver;
    char buf[1024];

    if (argc < 2) {
        std::cout << "ERROR, no port provided" << std::endl;
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Create a socket fd
    if (sockfd < 0) error("ERROR Opening socket");

    length = sizeof(struct sockaddr_in);
    memset((char *) &sender, 0, length);

    sender.sin_family = AF_INET;
    sender.sin_port = htons(atoi(argv[1]));
    sender.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &sender, length) < 0)
        error("binding issues");

    recv_len = sizeof(struct sockaddr_in);

    while (1) {
        n = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *) &receiver, &recv_len);
        if (n < 0) {
            // std::cout << "ERROR Sender Reading from socket" << std::endl;
            error("ERROR recvfrom");
        }
        // std::cout << "MESSAGE Received: " << buf << std::endl;
        write(1, "Recevied a datagram: ", 21);
        write(1, buf, n);
        n = sendto(sockfd, "Got your message\n", 17, 0, (struct sockaddr *)&receiver, recv_len);
        if (n < 0) {
            // std::cout << "ERROR Sender Sending from socket" << std::endl;
            error("ERROR sendto");
        }
    }

    return 0;
}
