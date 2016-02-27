#include "connection_manager.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

connection_manager::conection_manager(int win_size, double packet_loss, double packet_corrupt)
{
    memset(&sender_addr, 0, sizeof(struct sockaddr_in));
    memset(&receiver_addr, 0, sizeof(struct sockaddr_in));
}

bool connection_manager::connect(host_address, port)
{
}
