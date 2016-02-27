#include <sys/types.h> // >
#include <sys/socket.h> // ?
#include <netdb.h> // ?
#include <string>
#include <exception>

#define MAX_PACKET 1024
#define DEFAULT_WIN_SIZE 1024

class connection_manager {
    public:
        connection_manager(int win_size = DEFAULT_WIN_SIZE, double packet_loss = 0, double packet_corrupt = 0);

    private:
        sockaddr_in sender_addr;
        sockaddr_in receiver_addr;
        
};
