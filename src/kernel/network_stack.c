#include "kernel/network_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global network state
static Socket* sockets[MAX_SOCKETS];
static NetworkInterface interfaces[8];
static int interface_count = 0;
static RouteEntry routes[256];
static int route_count = 0;
static NetworkStats global_stats;
static int next_fd = 3; // Start after stdin/stdout/stderr

// Helper: Convert network byte order
static uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

static uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong & 0xFF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

static uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

static uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

// Initialize network stack
int netstack_init(void) {
    printf("[NetStack] Initializing network stack...\n");
    
    // Clear all state
    memset(sockets, 0, sizeof(sockets));
    memset(&interfaces, 0, sizeof(interfaces));
    memset(&routes, 0, sizeof(routes));
    memset(&global_stats, 0, sizeof(global_stats));
    interface_count = 0;
    route_count = 0;
    next_fd = 3;
    
    // Create loopback interface (lo)
    NetworkInterface* lo = &interfaces[interface_count++];
    strncpy(lo->name, "lo", sizeof(lo->name) - 1);
    lo->index = 0;
    lo->flags = 0x1; // IFF_UP
    memset(&lo->mac, 0, sizeof(MACAddress));
    lo->ip.octets[0] = 127;
    lo->ip.octets[1] = 0;
    lo->ip.octets[2] = 0;
    lo->ip.octets[3] = 1;
    lo->netmask.octets[0] = 255;
    lo->netmask.octets[1] = 0;
    lo->netmask.octets[2] = 0;
    lo->netmask.octets[3] = 0;
    lo->mtu = 65536;
    
    // Create main interface (zora0)
    NetworkInterface* eth0 = &interfaces[interface_count++];
    strncpy(eth0->name, "zora0", sizeof(eth0->name) - 1);
    eth0->index = 1;
    eth0->flags = 0x1; // IFF_UP
    // MAC: 52:54:00:12:34:56 (QEMU default range)
    eth0->mac.bytes[0] = 0x52;
    eth0->mac.bytes[1] = 0x54;
    eth0->mac.bytes[2] = 0x00;
    eth0->mac.bytes[3] = 0x12;
    eth0->mac.bytes[4] = 0x34;
    eth0->mac.bytes[5] = 0x56;
    // IP: 10.0.2.15 (VMware style)
    eth0->ip.octets[0] = 10;
    eth0->ip.octets[1] = 0;
    eth0->ip.octets[2] = 2;
    eth0->ip.octets[3] = 15;
    // Netmask: 255.255.255.0
    eth0->netmask.octets[0] = 255;
    eth0->netmask.octets[1] = 255;
    eth0->netmask.octets[2] = 255;
    eth0->netmask.octets[3] = 0;
    // Gateway: 10.0.2.1
    eth0->gateway.octets[0] = 10;
    eth0->gateway.octets[1] = 0;
    eth0->gateway.octets[2] = 2;
    eth0->gateway.octets[3] = 1;
    // Broadcast: 10.0.2.255
    eth0->broadcast.octets[0] = 10;
    eth0->broadcast.octets[1] = 0;
    eth0->broadcast.octets[2] = 2;
    eth0->broadcast.octets[3] = 255;
    eth0->mtu = 1500;
    
    // Add default route
    RouteEntry* default_route = &routes[route_count++];
    memset(&default_route->dest, 0, sizeof(IPv4Address));
    memset(&default_route->mask, 0, sizeof(IPv4Address));
    default_route->gateway = eth0->gateway;
    default_route->metric = 0;
    default_route->interface_id = 1;
    
    // Add local network route
    RouteEntry* local_route = &routes[route_count++];
    local_route->dest.octets[0] = 10;
    local_route->dest.octets[1] = 0;
    local_route->dest.octets[2] = 2;
    local_route->dest.octets[3] = 0;
    local_route->mask = eth0->netmask;
    memset(&local_route->gateway, 0, sizeof(IPv4Address));
    local_route->metric = 0;
    local_route->interface_id = 1;
    
    printf("[NetStack] Created interfaces: lo (127.0.0.1), zora0 (10.0.2.15)\n");
    printf("[NetStack] Default gateway: 10.0.2.1\n");
    
    return 0;
}

// Cleanup network stack
void netstack_cleanup(void) {
    printf("[NetStack] Cleaning up network stack...\n");
    
    // Close all sockets
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i]) {
            if (sockets[i]->recv_buffer) free(sockets[i]->recv_buffer);
            if (sockets[i]->send_buffer) free(sockets[i]->send_buffer);
            free(sockets[i]);
            sockets[i] = NULL;
        }
    }
}

// Create socket
int netstack_socket(int family, int type, int protocol) {
    if (family != AF_INET) {
        printf("[NetStack] Unsupported address family: %d\n", family);
        return -1;
    }
    
    // Find free socket slot
    int fd = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!sockets[i]) {
            fd = next_fd++;
            sockets[i] = (Socket*)calloc(1, sizeof(Socket));
            sockets[i]->fd = fd;
            sockets[i]->family = family;
            sockets[i]->type = type;
            sockets[i]->protocol = protocol;
            sockets[i]->state = SOCKET_CLOSED;
            
            // Allocate buffers (64KB each)
            sockets[i]->recv_buffer = (uint8_t*)malloc(65536);
            sockets[i]->recv_size = 65536;
            sockets[i]->send_buffer = (uint8_t*)malloc(65536);
            sockets[i]->send_size = 65536;
            
            // Random initial sequence number
            sockets[i]->seq_num = rand();
            
            printf("[NetStack] Created socket fd=%d (type=%d, proto=%d)\n", fd, type, protocol);
            return fd;
        }
    }
    
    printf("[NetStack] No available sockets\n");
    return -1;
}

// Bind socket
int netstack_bind(int sockfd, const SocketAddress* addr) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    // Check if port is already in use
    uint16_t port = ntohs(addr->port);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i] != sock && 
            ntohs(sockets[i]->local.port) == port) {
            printf("[NetStack] Port %d already in use\n", port);
            return -1;
        }
    }
    
    sock->local = *addr;
    printf("[NetStack] Bound socket fd=%d to port %d\n", sockfd, port);
    return 0;
}

// Listen on socket
int netstack_listen(int sockfd, int backlog) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    if (sock->type != SOCK_STREAM) {
        printf("[NetStack] Listen only supported on SOCK_STREAM\n");
        return -1;
    }
    
    sock->state = SOCKET_LISTEN;
    sock->backlog = (backlog > MAX_LISTEN_BACKLOG) ? MAX_LISTEN_BACKLOG : backlog;
    printf("[NetStack] Socket fd=%d listening (backlog=%d)\n", sockfd, sock->backlog);
    return 0;
}

// Accept connection
int netstack_accept(int sockfd, SocketAddress* addr) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock || sock->state != SOCKET_LISTEN) {
        printf("[NetStack] Socket not listening\n");
        return -1;
    }
    
    // Simulate accepting a connection (in real impl, would check accept_queue)
    int new_fd = netstack_socket(sock->family, sock->type, sock->protocol);
    if (new_fd < 0) return -1;
    
    Socket* new_sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == new_fd) {
            new_sock = sockets[i];
            break;
        }
    }
    
    if (new_sock) {
        new_sock->state = SOCKET_ESTABLISHED;
        new_sock->local = sock->local;
        // Simulate remote address
        new_sock->remote.family = AF_INET;
        new_sock->remote.port = htons(50000 + rand() % 15000);
        new_sock->remote.addr.octets[0] = 10;
        new_sock->remote.addr.octets[1] = 0;
        new_sock->remote.addr.octets[2] = 2;
        new_sock->remote.addr.octets[3] = 2 + rand() % 253;
        
        if (addr) *addr = new_sock->remote;
        
        global_stats.tcp_connections++;
        printf("[NetStack] Accepted connection fd=%d from %d.%d.%d.%d:%d\n",
               new_fd,
               new_sock->remote.addr.octets[0],
               new_sock->remote.addr.octets[1],
               new_sock->remote.addr.octets[2],
               new_sock->remote.addr.octets[3],
               ntohs(new_sock->remote.port));
    }
    
    return new_fd;
}

// Connect socket
int netstack_connect(int sockfd, const SocketAddress* addr) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    sock->remote = *addr;
    
    if (sock->type == SOCK_STREAM) {
        // TCP: Perform 3-way handshake
        sock->state = SOCKET_SYN_SENT;
        printf("[NetStack] Connecting fd=%d to %d.%d.%d.%d:%d (SYN sent)\n",
               sockfd,
               addr->addr.octets[0], addr->addr.octets[1],
               addr->addr.octets[2], addr->addr.octets[3],
               ntohs(addr->port));
        
        // Simulate handshake completion
        sock->state = SOCKET_ESTABLISHED;
        global_stats.tcp_connections++;
        printf("[NetStack] Connection fd=%d established\n", sockfd);
    } else {
        // UDP: Just set remote address
        printf("[NetStack] Connected fd=%d to %d.%d.%d.%d:%d (UDP)\n",
               sockfd,
               addr->addr.octets[0], addr->addr.octets[1],
               addr->addr.octets[2], addr->addr.octets[3],
               ntohs(addr->port));
    }
    
    return 0;
}

// Send data
int netstack_send(int sockfd, const void* data, uint32_t size, int flags) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    if (sock->state != SOCKET_ESTABLISHED) {
        printf("[NetStack] Socket not connected\n");
        return -1;
    }
    
    // Update statistics
    global_stats.packets_sent++;
    global_stats.bytes_sent += size;
    
    if (sock->type == SOCK_STREAM) {
        global_stats.tcp_connections++;
    } else if (sock->type == SOCK_DGRAM) {
        global_stats.udp_datagrams++;
    }
    
    printf("[NetStack] Sent %u bytes on fd=%d\n", size, sockfd);
    return size;
}

// Receive data
int netstack_recv(int sockfd, void* data, uint32_t size, int flags) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    // Simulate receiving data (in real impl, would read from recv_buffer)
    uint32_t bytes_received = (rand() % size) + 1;
    if (bytes_received > size) bytes_received = size;
    
    // Update statistics
    global_stats.packets_received++;
    global_stats.bytes_received += bytes_received;
    
    printf("[NetStack] Received %u bytes on fd=%d\n", bytes_received, sockfd);
    return bytes_received;
}

// Send datagram
int netstack_sendto(int sockfd, const void* data, uint32_t size, const SocketAddress* addr, int flags) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    // Update statistics
    global_stats.packets_sent++;
    global_stats.bytes_sent += size;
    global_stats.udp_datagrams++;
    
    printf("[NetStack] Sent %u bytes to %d.%d.%d.%d:%d\n",
           size,
           addr->addr.octets[0], addr->addr.octets[1],
           addr->addr.octets[2], addr->addr.octets[3],
           ntohs(addr->port));
    
    return size;
}

// Receive datagram
int netstack_recvfrom(int sockfd, void* data, uint32_t size, SocketAddress* addr, int flags) {
    Socket* sock = NULL;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            sock = sockets[i];
            break;
        }
    }
    
    if (!sock) {
        printf("[NetStack] Invalid socket fd=%d\n", sockfd);
        return -1;
    }
    
    // Simulate receiving datagram
    uint32_t bytes_received = (rand() % size) + 1;
    if (bytes_received > size) bytes_received = size;
    
    // Simulate source address
    if (addr) {
        addr->family = AF_INET;
        addr->port = htons(50000 + rand() % 15000);
        addr->addr.octets[0] = 10;
        addr->addr.octets[1] = 0;
        addr->addr.octets[2] = 2;
        addr->addr.octets[3] = 2 + rand() % 253;
    }
    
    // Update statistics
    global_stats.packets_received++;
    global_stats.bytes_received += bytes_received;
    global_stats.udp_datagrams++;
    
    printf("[NetStack] Received %u bytes from %d.%d.%d.%d:%d\n",
           bytes_received,
           addr->addr.octets[0], addr->addr.octets[1],
           addr->addr.octets[2], addr->addr.octets[3],
           ntohs(addr->port));
    
    return bytes_received;
}

// Close socket
int netstack_close(int sockfd) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->fd == sockfd) {
            Socket* sock = sockets[i];
            
            if (sock->state == SOCKET_ESTABLISHED && sock->type == SOCK_STREAM) {
                // TCP: Send FIN
                printf("[NetStack] Closing connection fd=%d (FIN sent)\n", sockfd);
                sock->state = SOCKET_FIN_WAIT_1;
            }
            
            // Free buffers
            if (sock->recv_buffer) free(sock->recv_buffer);
            if (sock->send_buffer) free(sock->send_buffer);
            
            // Free socket
            free(sock);
            sockets[i] = NULL;
            
            printf("[NetStack] Closed socket fd=%d\n", sockfd);
            return 0;
        }
    }
    
    printf("[NetStack] Invalid socket fd=%d\n", sockfd);
    return -1;
}

// Get interface
NetworkInterface* netstack_get_interface(int index) {
    if (index < 0 || index >= interface_count) return NULL;
    return &interfaces[index];
}

// Find interface by name
NetworkInterface* netstack_find_interface(const char* name) {
    for (int i = 0; i < interface_count; i++) {
        if (strcmp(interfaces[i].name, name) == 0) {
            return &interfaces[i];
        }
    }
    return NULL;
}

// Calculate checksum (RFC 1071)
uint16_t netstack_checksum(const void* data, uint32_t size) {
    const uint16_t* words = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (size > 1) {
        sum += *words++;
        size -= 2;
    }
    
    if (size > 0) {
        sum += *(const uint8_t*)words;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Parse IPv4 address
int netstack_parse_ipv4(const char* str, IPv4Address* addr) {
    int a, b, c, d;
    if (sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        return -1;
    }
    if (a < 0 || a > 255 || b < 0 || b > 255 ||
        c < 0 || c > 255 || d < 0 || d > 255) {
        return -1;
    }
    addr->octets[0] = a;
    addr->octets[1] = b;
    addr->octets[2] = c;
    addr->octets[3] = d;
    return 0;
}

// Format IPv4 address
void netstack_format_ipv4(const IPv4Address* addr, char* str, uint32_t size) {
    snprintf(str, size, "%d.%d.%d.%d",
             addr->octets[0], addr->octets[1],
             addr->octets[2], addr->octets[3]);
}

// Format MAC address
void netstack_format_mac(const MACAddress* mac, char* str, uint32_t size) {
    snprintf(str, size, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac->bytes[0], mac->bytes[1], mac->bytes[2],
             mac->bytes[3], mac->bytes[4], mac->bytes[5]);
}

// Get statistics
NetworkStats* netstack_get_stats(void) {
    return &global_stats;
}

// Dump statistics
void netstack_dump_stats(void) {
    printf("\n=== Network Statistics ===\n");
    printf("Packets sent:     %llu\n", (unsigned long long)global_stats.packets_sent);
    printf("Packets received: %llu\n", (unsigned long long)global_stats.packets_received);
    printf("Bytes sent:       %llu\n", (unsigned long long)global_stats.bytes_sent);
    printf("Bytes received:   %llu\n", (unsigned long long)global_stats.bytes_received);
    printf("Packets dropped:  %llu\n", (unsigned long long)global_stats.packets_dropped);
    printf("Errors:           %llu\n", (unsigned long long)global_stats.errors);
    printf("TCP connections:  %llu\n", (unsigned long long)global_stats.tcp_connections);
    printf("UDP datagrams:    %llu\n", (unsigned long long)global_stats.udp_datagrams);
    printf("ICMP messages:    %llu\n", (unsigned long long)global_stats.icmp_messages);
}

// Show active connections
void netstack_show_connections(void) {
    printf("\n=== Active Connections ===\n");
    printf("Proto  Local Address          Remote Address         State\n");
    
    int count = 0;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i] && sockets[i]->state != SOCKET_CLOSED) {
            Socket* sock = sockets[i];
            const char* proto = (sock->type == SOCK_STREAM) ? "TCP" : "UDP";
            const char* state = "";
            
            switch (sock->state) {
                case SOCKET_LISTEN: state = "LISTEN"; break;
                case SOCKET_SYN_SENT: state = "SYN_SENT"; break;
                case SOCKET_SYN_RECEIVED: state = "SYN_RECEIVED"; break;
                case SOCKET_ESTABLISHED: state = "ESTABLISHED"; break;
                case SOCKET_FIN_WAIT_1: state = "FIN_WAIT_1"; break;
                case SOCKET_FIN_WAIT_2: state = "FIN_WAIT_2"; break;
                case SOCKET_CLOSE_WAIT: state = "CLOSE_WAIT"; break;
                case SOCKET_CLOSING: state = "CLOSING"; break;
                case SOCKET_LAST_ACK: state = "LAST_ACK"; break;
                case SOCKET_TIME_WAIT: state = "TIME_WAIT"; break;
                default: state = "UNKNOWN"; break;
            }
            
            printf("%-6s %d.%d.%d.%d:%-5d    %d.%d.%d.%d:%-5d    %s\n",
                   proto,
                   sock->local.addr.octets[0], sock->local.addr.octets[1],
                   sock->local.addr.octets[2], sock->local.addr.octets[3],
                   ntohs(sock->local.port),
                   sock->remote.addr.octets[0], sock->remote.addr.octets[1],
                   sock->remote.addr.octets[2], sock->remote.addr.octets[3],
                   ntohs(sock->remote.port),
                   state);
            count++;
        }
    }
    
    if (count == 0) {
        printf("(No active connections)\n");
    }
}

// ICMP ping
int netstack_icmp_ping(const IPv4Address* dest, uint16_t id, uint16_t seq) {
    printf("[NetStack] Sending ICMP echo request to %d.%d.%d.%d (id=%d, seq=%d)\n",
           dest->octets[0], dest->octets[1], dest->octets[2], dest->octets[3],
           id, seq);
    
    // Simulate RTT
    int rtt_ms = 1 + (rand() % 50);
    
    global_stats.packets_sent++;
    global_stats.packets_received++;
    global_stats.icmp_messages += 2;
    
    printf("[NetStack] Received ICMP echo reply (RTT=%d ms)\n", rtt_ms);
    return rtt_ms;
}

// Show routes
void netstack_show_routes(void) {
    printf("\n=== Routing Table ===\n");
    printf("Destination     Gateway         Netmask         Interface  Metric\n");
    
    for (int i = 0; i < route_count; i++) {
        RouteEntry* route = &routes[i];
        NetworkInterface* iface = netstack_get_interface(route->interface_id);
        
        char dest_str[16], gw_str[16], mask_str[16];
        netstack_format_ipv4(&route->dest, dest_str, sizeof(dest_str));
        netstack_format_ipv4(&route->gateway, gw_str, sizeof(gw_str));
        netstack_format_ipv4(&route->mask, mask_str, sizeof(mask_str));
        
        printf("%-15s %-15s %-15s %-10s %d\n",
               dest_str, gw_str, mask_str,
               iface ? iface->name : "?", route->metric);
    }
}
