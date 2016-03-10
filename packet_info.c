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
    int crc_cksum;
    /* Status
     * 0: Middle of file
     * 1: Finished
     */
    int status;
    /* Health
     * 0: Good
     * 1: Loss
     * 2: Corrupt
     */
    int health;
    double time;
    char data[data_MTU];
    int data_size;
//     int length;
};

void print_pkt_info(struct packet_info packet)
{
    printf("--------------------------------------------------\n");
    printf("Data   Type:\t\t%d\t(1: DATA, 2: ACK, 3: Retransmission)\n", packet.type);
    printf("Resp.  Stat:\t\t%d\t(0: Packets being transmitted. 1: Last packet)\n", packet.status);
    printf("H e a l t h:\t\t%d\t(0: Good. 1: Packet Loss. 2: Packet Corrupt)\n", packet.health);
    printf("Sequence No:\t\t%d\n", packet.seq_no);
    printf("Sequence Max:\t\t%d\n", packet.max_no);
    printf("Data   Time:\t\t%f\n", packet.time);
    printf("CRC  CHKSUM:\t\t%x\n", packet.crc_cksum);
    printf("\n");
};