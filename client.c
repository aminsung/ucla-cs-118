#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "packet_info.c"

// int MTU = 256;
double P_c = 0.0;
double P_l = 0.0;

#define CRC16 0x8005

int diff_ms(struct timeval t1, struct timeval t2)
{
    return (((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec))/1000;
}


double random_threshold(int t)
{
    // Random threshold to see if the loss/corruption stays below this!
    srand(t);
    return ((double) rand() / (double) RAND_MAX);
};

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

int main(int argc, char *argv[])
{
    /*
    if (argc != 6) error("ERROR Missing parameters.\n\rFormat: <sender_hostname> <sender_portnumber> <filename> <packet_loss_value> <packet_corrupt_value>");
    */

    struct timeval start;
    gettimeofday(&start, NULL);
    int sockfd, n, seq_no;
    unsigned int length;
    struct sockaddr_in sender, receiver; // sender: server | receiver: client
    struct hostent *hp;
    char *filename;
    double pkt_loss_prob, pkt_corrupt_prob;
    // Check to see
    // Up to packet loss argument needed! Packet corruption not implementedall arguments are given
    if (argc != 7)
        perror("Not all arguments given\n\n");

    // Store requested filename and packet loss percentage
    filename = argv[3];

    // NOTE Make sure to change the argv[] values to follow the format of the guidelines

    int CW = atoi(argv[4]);
    int window_size = CW / data_MTU;

    pkt_loss_prob = atof(argv[5]);
    pkt_corrupt_prob = atof(argv[6]);
    int i = 0;
    int lost_count;
    if (pkt_loss_prob != 0.0)
        lost_count = 1 / pkt_loss_prob;
    int corrupt_count;
    if (pkt_corrupt_prob != 0.0)
        corrupt_count = 1 / pkt_corrupt_prob;

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

    /* This is the requested file that is transferred over */
    FILE *recv_file;
    recv_file = fopen(strcat(filename, "_(2)"), "wb");

    /* Prep for the response packets */
    struct packet_info response_packet;
    memset((char *) &response_packet, 0, sizeof(response_packet));

    response_packet.status = 0;
    response_packet.seq_no = 0;

    struct packet_info *buffer;
    buffer = (struct packet_info *) malloc(window_size * sizeof(struct packet_info));
    for (i = 0; i<window_size; i++)
        memset(&(buffer[i]), -1, sizeof(struct packet_info));

    int start_of_seq = 0;
    int end_of_seq = window_size;
    while(1)
    {
        recvfrom(sockfd, &response_packet, sizeof(response_packet), 0, (struct sockaddr *) &receiver, &length);
        
        struct timeval end;
        gettimeofday(&end, NULL);
        int time_diff = diff_ms(end, start);
        double random = random_threshold(time_diff); 
        /* Get the max # of sequence */
        if (end_of_seq > response_packet.max_no)
            end_of_seq = response_packet.max_no;
        else
            end_of_seq = window_size;

        /* Loss and Corruption */
        if (pkt_loss_prob != 0.0)
            if (pkt_loss_prob > random){
            // if (i % lost_count == 0){
                memset(response_packet.data, '0', response_packet.data_size);
                response_packet.health = 1;
            }

        if (pkt_corrupt_prob != 0.0)
            if (pkt_corrupt_prob < random
                && random < (pkt_corrupt_prob+pkt_loss_prob)){
            // if (i % corrupt_count == 1){
            int i;
            for (i = 0; i<response_packet.data_size; i++)
            {
                response_packet.data[i] = 1+response_packet.data[i];
                response_packet.health = 2;
            }
            
        }

        int current_pkt = response_packet.seq_no;
        // 1. pkt # in [rcvbase, rcvbase+N-1]
        if ((current_pkt - start_of_seq) >= 0
            && (current_pkt - start_of_seq) < window_size){
            // send ACK
            response_packet.type = 2;
            int crc_result = gen_crc16(response_packet.data, response_packet.data_size);
            sendto(sockfd, &crc_result, sizeof(crc_result), 0, (struct sockaddr *)&receiver, length);
            // buffer first
            memcpy(&(buffer[current_pkt - start_of_seq]), &response_packet, sizeof(struct packet_info));

            while (1){
                int crc_ret = gen_crc16(buffer[0].data, buffer[0].data_size);

                // only in-order is not enough
                // you have to be right
                if (buffer[0].seq_no == start_of_seq
                    && crc_ret == buffer[0].crc_cksum){
                    //printf("%d\t%x\t%d\n", buffer[0].seq_no, crc_ret, buffer[0].max_no);
                    //print_pkt_info(buffer[0]);
                    fwrite(buffer[0].data, sizeof(char), buffer[0].data_size, recv_file);

                    for (i = 0; i<window_size-1; i++){
                        memcpy(&(buffer[i]), &(buffer[i+1]), sizeof(struct packet_info));
                    }
                    memset(&(buffer[window_size-1]), -1, sizeof(struct packet_info));
                    start_of_seq++;
                }
                else
                    break;
            }
        }
        // 2. pkt # in [rcvbase-N,rcvbase-1]
        else if ((current_pkt - start_of_seq) >= -window_size
            && (current_pkt - start_of_seq) < 0){
            // send ACK
            response_packet.type = 2;
            int crc_result = gen_crc16(response_packet.data, response_packet.data_size);
            sendto(sockfd, &crc_result, sizeof(crc_result), 0, (struct sockaddr *)&receiver, length);
        }

        print_pkt_info(response_packet);


        //printf("\nSeq Order: %d\n", response_packet.seq_no);
        //printf("CRC result: %x\n\n", crc_result);
        memset((char *) &response_packet.data, 0, sizeof(response_packet.data));

        if (response_packet.status == 1)
            break;
        i++;
    }

    close(sockfd);
    return 0;
}
