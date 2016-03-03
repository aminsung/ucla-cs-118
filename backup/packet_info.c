#define MTU 1024

struct packet_info
{
    int data_type;
    int seq_no;
    int status;
    double time;
    char *data;
};
