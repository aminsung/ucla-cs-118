#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include "packet_info.c"

#define CRC16 0x8005
#define WAIT 3000               // wait time in ms

int diff_ms(struct timeval t1, struct timeval t2)
{
    return (((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec))/1000;
}

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
    int CW;
    

    if (argc < 3) {
        error("ERROR, not enough arguments\n");
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

    CW = atoi(argv[2]);
    int window_size = CW / data_MTU;

    struct timeval start;
    gettimeofday(&start, NULL);
    double time_last = 0.0;

    while (1) {
        struct timeval end;
        gettimeofday(&end, NULL);
        double time_diff = diff_ms(end, start);

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
        int start_of_seq = 0;
        int end_of_seq;
        if (window_size > no_of_pkt)
            end_of_seq = no_of_pkt;
        else
            end_of_seq = window_size;

        //printf("Total File Size: %d bytes\n\n", fsize(file_stream));
        //printf("Total Packet No.: %d packets\n\n", no_of_pkt);
        struct packet_info *packet_window;
        packet_window = (struct packet_info *) malloc(window_size * sizeof(struct packet_info));
        
        int* crc_table = (int*) malloc(window_size*sizeof(int));
        // Create partitioned packets
        for(response_packet.seq_no = start_of_seq;
            response_packet.seq_no < end_of_seq;)
        {
            // packet properties: type/data/status/seq_no
            // Data packet
            int crc_result;
            {
                response_packet.type = 1; 
                //printf("Seq Order: %d\n", response_packet.seq_no);

                // read packet
                n2 = fread(response_packet.data, sizeof(char), data_MTU, file_stream);
                
                // Last packet
                if (response_packet.seq_no >= response_packet.max_no-1)
                    response_packet.status = 1;
                else
                    response_packet.status = 0;
                response_packet.data_size = n2;


                // checksum packet
                crc_result = gen_crc16(response_packet.data, n2);

                // both checksum array and the packet array.
                crc_table[response_packet.seq_no - start_of_seq] = crc_result;
                memcpy(&(packet_window[response_packet.seq_no - start_of_seq]), &response_packet, sizeof(struct packet_info));
            }

            //printf("Fread Bytes: %d\n\n", n2);
            //printf("CRC result: %x\n\n", crc_result);
            sendto(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *)&receiver, recv_len);
            
            // response
            int received_crc;
            recvfrom(sockfd, &received_crc, sizeof(received_crc), 0, (struct sockaddr *) &receiver, &recv_len);

            // 2b. check if anything received;
            int i, check_received = 0;
            for (i = start_of_seq; i<end_of_seq; i++)
                if (crc_table[i-start_of_seq] == received_crc)
                    crc_table[i-start_of_seq] = -1;

            // 2a. if the first of window received
            if (crc_table[0] != -1)
                ;//printf("CRC result: %x\t%x\t%d\n", crc_result, received_crc, response_packet.seq_no);
            else{
                printf("CRC result: %x\t%x\t%d\n", crc_result, received_crc, response_packet.seq_no);
                response_packet.seq_no++;
                for (i = 0; i<window_size-1; i++){
                    crc_table[i] = crc_table[i+1];
                    // just a dump function :)
                    //print_pkt_info(packet_window[i]);
                    memcpy(&(packet_window[i]), &(packet_window[i+1]), sizeof(struct packet_info));
                }
                crc_table[window_size-1] = 0;
                memset(&(packet_window[window_size-1]), 0, sizeof(struct packet_info));
                start_of_seq++;

                if (end_of_seq<no_of_pkt)
                    end_of_seq++;
            }
        }

        fclose(file_stream);
        //printf("Finished transmitting...\n\n");
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
