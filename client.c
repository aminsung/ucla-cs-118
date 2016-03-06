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

// int MTU = 256;
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
    double pkt_loss_prob, pkt_corrupt_prob;

    // Check to see
    // Up to packet loss argument needed! Packet corruption not implementedall arguments are given
    if (argc != 6)
        perror("Not all arguments given\n\n");

    // Store requested filename and packet loss percentage
    filename = argv[3];

    // NOTE Make sure to change the argv[] values to follow the format of the guidelines
    pkt_loss_prob = atof(argv[4]);
    pkt_corrupt_prob = atof(argv[5]);

    // Create socket fd
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERROR Opening socket");

    // Setup server info
    hp = gethostbyname(argv[1]);
    if (hp==0) error("ERROR Unknown host");

    sender.sin_family = AF_INET;
    bcopy((char *)hp->h_addr, (char *)&sender.sin_addr, hp->h_length);
    sender.sin_port = htons(atoi(argv[2]));

    // Construct a request packet to send
    struct packet_info request_packet;
    // bzero((char *) &request_packet, sizeof(request_packet));
    memset((char *) &request_packet, 0, sizeof(request_packet));
    strcpy(request_packet.data, filename);
    
    // Create request packet to request the file
    length = sizeof(struct sockaddr_in);
    request_packet.seq_no = 1;
    // request_packet.length = sizeof(request_packet);

    // Send initial request packet
    n = sendto(sockfd, &request_packet, sizeof(request_packet), 0, (const struct sockaddr *)&sender, length);
    if (n < 0) error("ERROR Sendto");
    print_pkt_info(request_packet);

    // This is the requested file that is transferred over
    FILE *recv_file;
    recv_file = fopen(strcat(filename, "_recv"), "wb");

    // Prep for the respones packet
    struct packet_info response_packet;
    memset((char *) &response_packet, 0, sizeof(response_packet));

    n = recvfrom(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *) &receiver, &length);
    if (n < 0) error("ERROR recvfrom");
    response_packet.status = 0;
    print_pkt_info(response_packet);

    while(1)
    {
        n = recvfrom(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *) &receiver, &length);
        fwrite(response_packet.data, sizeof(char), data_MTU, recv_file);
        print_pkt_info(response_packet);

        if (n < 0 || pkt_loss_prob < random_threshold())
        {
            // printf("\nERROR PACKET LOSS!\n");
        }
        else if (pkt_corrupt_prob < random_threshold())
        {
            // printf("\nERROR PACKET CORRUPT!\n");
        } 

        if (response_packet.status == 1)
            break;

    }

    close(sockfd);
    return 0;
}
