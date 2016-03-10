#define data_MTU 512 

// Packet struct comes out to be 544 bytes with data_MTU of 512
struct packet_info
{
    /* Packet Types
     * DATA: 1
     * ACK: 2
     * Retransmission: 3
     */
    int type;
    int seq_no;
    int max_no;

    /* Status
     * 0: Middle of file
     * 1: Finished
     */
    int status;
    double time;
    char data[data_MTU];
    int data_size;
    int length;
};

void print_pkt_info(struct packet_info packet)
{
    printf("--------------------------------------------------\n");
    printf("Data   Type:\t\t%d\n", packet.type);
    printf("Resp.  Stat:\t\t%d\n", packet.status);
    printf("Sequence No:\t\t%d\n", packet.seq_no);
    printf("Packet Data:\n%s\n", packet.data);
    printf("\n");
};

double random_threshold(int t)
{
    // Random threshold to see if the loss/corruption stays below this!
    srand(t);
    return ((double) rand() / (double) RAND_MAX);
};
