#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "packet_info.c"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// void fetch_file(char *requested_file, char *buf)
void fetch_file(char *requested_file, char *buff_data)
{
    // Create an absolute path to the requested file
    FILE* fd;
    char *home = getenv("PWD");
    char *full_path = malloc(strlen(home) + strlen(requested_file) + 1);
    strcpy(full_path, home);
    strcat(full_path, "/");
    strcat(full_path, requested_file);

    // char *buff_data = NULL; // Where the file that is requested will be stored
    if ( (fd = fopen(full_path, "r"))!= NULL)
    {
        int file_size = fsize(fd);
        // buff_data = malloc(sizeof(char)*(file_size+1));
        size_t new_file_len = fread(buff_data, sizeof(char), file_size, fd);
        buff_data[new_file_len++] = '\0';
    }
    else
    {
        perror("Cannot open file");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, length, n;
    socklen_t recv_len;
    struct sockaddr_in sender, receiver;
    struct packet_info request_packet, response_packet;
    char buf[MTU];
    

    if (argc < 2) {
        error("ERROR, no port provided\n");
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
    memset((char *) &request_packet, 0, sizeof(request_packet));
    request_packet.length = sizeof(request_packet);


    while (1) {
        n = recvfrom(sockfd, &request_packet, sizeof(request_packet), 0, (struct sockaddr *) &receiver, &recv_len);
        if (n < 0) error("ERROR recvfrom");

        // Print request packet data
        print_pkt_info(request_packet);


        // Setup & send back a response packet
        memset((char *) &response_packet, 0, sizeof(response_packet));
        response_packet.type = 2;
        response_packet.seq_no = request_packet.seq_no;

        // Fetch the file and store in the response packet
        fetch_file(request_packet.data, response_packet.data);

        // Uncomment this to see that the file has actually been stored in the struct. Definitely bigger than 1Kbyte tho.
        // printf("%s", response_packet.data);
        // printf("\n\n\n%lu\n\n\n", sizeof(response_packet.data));

        n = sendto(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *)&receiver, recv_len); 
        if (n < 0) error("ERROR sendto");
    }

    return 0;
}

// Function for finding the length of a file
int fsize(FILE *fp)
{
    int prev = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}
