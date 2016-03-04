#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "packet_info.c"

int data_size = 256;
double P_c = 0.0;
double P_l = 0.0;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int random_status(double probability)
{
    if ((double) rand() / (double) RAND_MAX)
       return 1;
    return 0; 
};

void print_pkt_info(struct packet_info packet)
{
    // printf("Packet Type: %s\t Seq No.: %d\t Corrupt?: %d\t Lost?: %d\t Timer Val: %d");
    printf("%d", packet.data_type);
};

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
    char buf[data_size];
    double pkt_loss_prob;

    filename = argv[3];
    pkt_loss_prob = htons(atoi(argv[4]));

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
    bzero((char *) &request_packet, sizeof(request_packet));
    strcpy(request_packet.data, filename);
    
    length = sizeof(struct sockaddr_in);
    request_packet.length = sizeof(request_packet);
    n = sendto(sockfd, &request_packet.data, request_packet.length, 0, (const struct sockaddr *)&sender, length);
    if (n < 0) error("ERROR Sendto");
    // printf("EVENT: File %s has been requested.\n\n", request_packet.data);
    print_pkt_info(request_packet);













    n = sendto(sockfd, buf, strlen(buf), 0, (const struct sockaddr *)&sender, length);
    if (n < 0) error("ERROR Sendto");
    n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *)&receiver, &length);
    if (n < 0) error("ERROR recvfrom");
    write(1, "Got an ack: ", 12);
    write(1, buf, n);
    close(sockfd);
    return 0;
}
