#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include "packet_info.h"

int MTU = 256;
double P_c = 0.0;
double P_l = 0.0;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    /*
    if (argc != 6) error("ERROR Missing parameters.\n\rFormat: <sender_hostname> <sender_portnumber> <filename> <packet_loss_value> <packet_corrupt_value>");
    */

    int sockfd, n, seq_no;
    unsigned int length;
    struct sockaddr_in sender, receiver; // sender: server | receiver: client
    struct hostent *hp;
    char *filename;
    char buf[MTU];

    filename = argv[3];

    // Create socket fd
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERROR Opening socket");

    hp = gethostbyname(argv[1]);
    if (hp==0) error("ERROR Unknown host");

    sender.sin_family = AF_INET;
    bcopy((char *)hp->h_addr, (char *)&sender.sin_addr, hp->h_length);
    sender.sin_port = htons(atoi(argv[2]));

    // Construct a request packet to send
    struct packet_info request_packet;
    request_packet..data = 
    
    length = sizeof(struct sockaddr_in);
    printf("Please enter the message: ");
    bzero(buf, 256);
    // memset((char *) &   
    fgets(buf, 255, stdin);
    n = sendto(sockfd, buf, strlen(buf), 0, (const struct sockaddr *)&sender, length);
    if (n < 0) error("ERROR Sendto");
    n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *)&receiver, &length);
    if (n < 0) error("ERROR recvfrom");
    write(1, "Got an ack: ", 12);
    write(1, buf, n);
    close(sockfd);
    return 0;
}
