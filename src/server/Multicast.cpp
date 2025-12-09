#include "Multicast.h"

// Ingressa no grupo multicast de comunicação entre as réplicas
void Multicast::init() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               &reuse, sizeof(reuse));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(12345);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock,
             (sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind");
        return;
    }

    ip_mreq mreq{};
    inet_pton(AF_INET, "239.0.0.1", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP,
                   IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        perror("IP_ADD_MEMBERSHIP");
        return;
    }
}