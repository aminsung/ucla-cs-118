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

#define CRC16 0x8005

uint16_t gen_crc16(const uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    /* Sanity check: */
    if(data == NULL)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> (7 - bits_read)) & 1;

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;

    }
    return out;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int fsize(FILE *fp);

int main(int argc, char *argv[])
{
    int sockfd, length, n, n2;
    socklen_t recv_len;
    struct sockaddr_in sender, receiver;
    struct packet_info request_packet, response_packet;
    

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


    while (1) {
        n = recvfrom(sockfd, &request_packet, sizeof(request_packet), 0, (struct sockaddr *) &receiver, &recv_len);
        if (n < 0) error("ERROR recvfrom");

        // Print request packet data
        print_pkt_info(request_packet);

        // Find the absolute path to the requested file; path is stored in 'full_path'
        FILE *file_stream;
        char *home = getenv("PWD");
        char *full_path = malloc(strlen(home) + strlen(request_packet.data) + 1);
        strcpy(full_path, home);
        strcat(full_path, "/");
        strcat(full_path, request_packet.data);

        if ( (file_stream = fopen(full_path, "rb")) == NULL )
            perror("Cannot open file");

        // Setup to send back a response packet
        memset((char *) &response_packet, 0, sizeof(response_packet));

        // Determines how many packets we have to send, based on the length of the file that is requested
        int no_of_pkt = (fsize(file_stream) + (data_MTU-1)) / data_MTU;
        response_packet.max_no = no_of_pkt;
        response_packet.seq_no = 0;
        printf("Total File Size: %d bytes\n\n", fsize(file_stream));
        printf("Total Packet No.: %d packets\n\n", no_of_pkt);

        // Create partitioned packets
        while (response_packet.seq_no < no_of_pkt)
        {
            response_packet.type = 1; // Data packet
            response_packet.seq_no++; // Sequence (ACK) number
            printf("Seq Order: %d\n", response_packet.seq_no);

            n2 = fread(response_packet.data, sizeof(char), data_MTU, file_stream);

            if (response_packet.seq_no >= response_packet.max_no) // Last packet
                response_packet.status = 1;
            else
                response_packet.status = 0;
            int crc_result;
            crc_result = gen_crc16(response_packet.data, n2);
            printf("Fread Bytes: %d\n\n", n2);
            printf("CRC result: %x\n\n", crc_result);
            response_packet.data_size = n2;
            sendto(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *)&receiver, recv_len);
        }

        fclose(file_stream);
        printf("Finished transmitting...\n\n");
        return 0;
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
