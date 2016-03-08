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
#define WAIT 300               // wait time in ms

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
    

    if (argc < 5) {
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
    double pkt_loss_prob, pkt_corrupt_prob;
    pkt_loss_prob = atof(argv[3]);
    pkt_corrupt_prob = atof(argv[4]);

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
        int start_of_seq = 0;
        int current_pkt = 0;
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
        int* time_table = (int*) malloc(window_size*sizeof(int));
        int i;
        for (i = 0; i<window_size; i++)
            time_table[i] = -2;
        // -2 for not yet sent
        // -1 for already ACK'ed
        // o.w. for not yet ACK'ed.

        // Create partitioned packets
        while(1){
            // 1a) if next available seq # in window, send pkt
            if (time_table[current_pkt - start_of_seq] == -2){

                // data packet
                response_packet.type = 1;

                // read packet from file
                n2 = fread(response_packet.data, sizeof(char), data_MTU, file_stream);
                response_packet.seq_no = current_pkt;
                response_packet.max_no = no_of_pkt;

                // check if last packet
                if (response_packet.seq_no >= response_packet.max_no-1 
                &&  start_of_seq == end_of_seq - 1)
                    response_packet.status = 1;
                else
                    response_packet.status = 0;

                // set response size
                response_packet.data_size = n2;

                // set crc table
                int crc_result = gen_crc16(response_packet.data, n2);
                crc_table[current_pkt - start_of_seq] = crc_result;
                response_packet.crc_cksum = crc_result;
                //printf("%x\t", crc_result);
                // set time table
                struct timeval end;
                gettimeofday(&end, NULL);
                int time_diff = diff_ms(end, start);
                time_table[current_pkt - start_of_seq] = time_diff;

                // save it
                memcpy(&(packet_window[current_pkt - start_of_seq]), &response_packet, sizeof(struct packet_info));

                // send it
                sendto(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *)&receiver, recv_len);
                if (current_pkt < end_of_seq)
                    current_pkt++;
            }
            // 2a) resend pkt n, restart timer
            
            for (i = start_of_seq; i<end_of_seq; i++){
                if (time_table[i-start_of_seq] >= 0){
                    struct timeval end;
                    gettimeofday(&end, NULL);
                    int time_diff = diff_ms(end, start);
                    if (time_diff - time_table[i-start_of_seq] > WAIT){
                        //printf("(%d)\n", time_diff - time_table[i-start_of_seq]);
                        sendto(sockfd, &(packet_window[i - start_of_seq]), sizeof((packet_window[i - start_of_seq])), 0, (struct sockaddr *)&receiver, recv_len);
                        time_table[i-start_of_seq] = diff_ms(end, start);
                    }
                }
            }
            

            // 3a) ACK(n) in [sendbase,sendbase+N]: mark pkt n as received

            // initialize variables
            fd_set inSet;
            struct timeval timeout;
            int received;

            FD_ZERO(&inSet);
            FD_SET(sockfd, &inSet); 

            // wait for specified time
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;

            // check for acks
            received = select(sockfd+1, &inSet, NULL, NULL, &timeout);

            // if timeout and no acks were received, break and maintain window position
            if(received < 1)
                continue;
            // otherwise, fetch the ack
            int received_crc;
            recvfrom(sockfd, &received_crc, sizeof(received_crc), 0, (struct sockaddr *) &receiver, &recv_len);
            /*printf("%x\t%d\n", received_crc, current_pkt);
            for (i = start_of_seq; i<end_of_seq; i++){
                printf("%d\t", time_table[i-start_of_seq]);
            }
            printf("\n");*/
            for (i = start_of_seq; i<end_of_seq; i++){
                if (crc_table[i-start_of_seq] == received_crc)
                    time_table[i-start_of_seq] = -1;
            }


            // 3b
            if (time_table[0] == -1){
                for (i = 0; i<window_size-1; i++){
                    crc_table[i] = crc_table[i+1];
                    time_table[i] = time_table[i+1];
                    memcpy(&(packet_window[i]), &(packet_window[i+1]), sizeof(struct packet_info));
                }
                crc_table[window_size-1] = 0;
                time_table[window_size-1] = -2;
                memset(&(packet_window[window_size-1]), 0, sizeof(struct packet_info));
                start_of_seq++;

                if (end_of_seq<no_of_pkt)
                    end_of_seq++;
                window_size = end_of_seq - start_of_seq;
            }
            if (start_of_seq == end_of_seq)
                break;
            else
                continue;
        }
        fclose(file_stream);
        printf("Finished transmitting...\n\n");
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
