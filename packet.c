#define MTU 1024

struct packet_info
{
    // DATA, ACK, or Retransmit
    int type;

    int seq_no;
    int status;
    double time;
    char data[MTU];
    int length;
};

void print_pkt_info(struct packet_info packet)
{
    printf("--------------------------------------------------\n");
    printf("  Data   Type:\t\t%d\n", packet.type);
    printf("  Sequence No:\t\t%d\n", packet.seq_no);
    printf("  Packet Data:\t\t%s\n", packet.data);
    printf("\n");
};

double random_threshold()
{
    // Random threshold to see if the loss/corruption stays below this!
    return ((double) rand() / (double) RAND_MAX);
};
