#ifndef KERNEL_NETWORK_STACK_H
#define KERNEL_NETWORK_STACK_H

#include <stdint.h>

// Protocol numbers (from RFC)
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

// Socket types
#define SOCK_STREAM     1   // TCP
#define SOCK_DGRAM      2   // UDP
#define SOCK_RAW        3   // Raw IP

// Address families
#define AF_INET         2   // IPv4
#define AF_INET6        10  // IPv6

// Socket states
#define SOCKET_CLOSED       0
#define SOCKET_LISTEN       1
#define SOCKET_SYN_SENT     2
#define SOCKET_SYN_RECEIVED 3
#define SOCKET_ESTABLISHED  4
#define SOCKET_FIN_WAIT_1   5
#define SOCKET_FIN_WAIT_2   6
#define SOCKET_CLOSE_WAIT   7
#define SOCKET_CLOSING      8
#define SOCKET_LAST_ACK     9
#define SOCKET_TIME_WAIT    10

// Maximum sockets
#define MAX_SOCKETS         256
#define MAX_CONNECTIONS     64
#define MAX_LISTEN_BACKLOG  128

// Packet sizes
#define ETH_FRAME_SIZE      1518
#define IP_PACKET_SIZE      65535
#define TCP_SEGMENT_SIZE    1460
#define UDP_DATAGRAM_SIZE   65507

// Port ranges
#define PORT_MIN            1
#define PORT_MAX            65535
#define PORT_PRIVILEGED     1024

// IPv4 address structure
typedef struct {
    uint8_t octets[4];
} IPv4Address;

// MAC address structure
typedef struct {
    uint8_t bytes[6];
} MACAddress;

// Socket address
typedef struct {
    uint16_t family;        // AF_INET, etc.
    uint16_t port;          // Port number (network byte order)
    IPv4Address addr;       // IP address
    uint8_t padding[8];     // Padding for compatibility
} SocketAddress;

// Network interface
typedef struct {
    char name[16];          // Interface name (eth0, lo, etc.)
    int index;              // Interface index
    int flags;              // IFF_UP, IFF_BROADCAST, etc.
    MACAddress mac;         // Hardware address
    IPv4Address ip;         // IP address
    IPv4Address netmask;    // Netmask
    IPv4Address broadcast;  // Broadcast address
    IPv4Address gateway;    // Default gateway
    uint64_t rx_packets;    // Received packets
    uint64_t tx_packets;    // Transmitted packets
    uint64_t rx_bytes;      // Received bytes
    uint64_t tx_bytes;      // Transmitted bytes
    int mtu;                // Maximum transmission unit
} NetworkInterface;

// Ethernet frame
typedef struct {
    MACAddress dest;        // Destination MAC
    MACAddress src;         // Source MAC
    uint16_t type;          // EtherType (0x0800 = IPv4, 0x0806 = ARP)
    uint8_t data[ETH_FRAME_SIZE - 14];
} __attribute__((packed)) EthernetFrame;

// IP header
typedef struct {
    uint8_t version_ihl;    // Version (4 bits) + IHL (4 bits)
    uint8_t tos;            // Type of service
    uint16_t total_length;  // Total length
    uint16_t id;            // Identification
    uint16_t flags_offset;  // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t ttl;            // Time to live
    uint8_t protocol;       // Protocol (TCP=6, UDP=17, ICMP=1)
    uint16_t checksum;      // Header checksum
    IPv4Address src_addr;   // Source IP
    IPv4Address dest_addr;  // Destination IP
} __attribute__((packed)) IPv4Header;

// TCP header
typedef struct {
    uint16_t src_port;      // Source port
    uint16_t dest_port;     // Destination port
    uint32_t seq_num;       // Sequence number
    uint32_t ack_num;       // Acknowledgment number
    uint8_t offset_reserved;// Data offset (4 bits) + Reserved (4 bits)
    uint8_t flags;          // TCP flags (SYN, ACK, FIN, etc.)
    uint16_t window;        // Window size
    uint16_t checksum;      // Checksum
    uint16_t urgent_ptr;    // Urgent pointer
} __attribute__((packed)) TCPHeader;

// TCP flags
#define TCP_FIN     0x01
#define TCP_SYN     0x02
#define TCP_RST     0x04
#define TCP_PSH     0x08
#define TCP_ACK     0x10
#define TCP_URG     0x20

// UDP header
typedef struct {
    uint16_t src_port;      // Source port
    uint16_t dest_port;     // Destination port
    uint16_t length;        // Length (header + data)
    uint16_t checksum;      // Checksum
} __attribute__((packed)) UDPHeader;

// ICMP header
typedef struct {
    uint8_t type;           // Message type
    uint8_t code;           // Message code
    uint16_t checksum;      // Checksum
    uint16_t id;            // Identifier
    uint16_t sequence;      // Sequence number
} __attribute__((packed)) ICMPHeader;

// Socket structure
typedef struct {
    int fd;                 // File descriptor
    int family;             // Address family (AF_INET)
    int type;               // Socket type (SOCK_STREAM, SOCK_DGRAM)
    int protocol;           // Protocol (IPPROTO_TCP, IPPROTO_UDP)
    int state;              // Socket state
    SocketAddress local;    // Local address/port
    SocketAddress remote;   // Remote address/port
    int backlog;            // Listen backlog
    void* accept_queue;     // Queue of pending connections
    uint8_t* recv_buffer;   // Receive buffer
    uint32_t recv_size;     // Receive buffer size
    uint32_t recv_head;     // Receive buffer head
    uint32_t recv_tail;     // Receive buffer tail
    uint8_t* send_buffer;   // Send buffer
    uint32_t send_size;     // Send buffer size
    uint32_t send_head;     // Send buffer head
    uint32_t send_tail;     // Send buffer tail
    int flags;              // Socket flags
    uint32_t seq_num;       // TCP sequence number
    uint32_t ack_num;       // TCP acknowledgment number
} Socket;

// Routing table entry
typedef struct {
    IPv4Address dest;       // Destination network
    IPv4Address mask;       // Network mask
    IPv4Address gateway;    // Gateway IP
    int metric;             // Route metric
    int interface_id;       // Interface index
} RouteEntry;

// Network statistics
typedef struct {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_dropped;
    uint64_t errors;
    uint64_t tcp_connections;
    uint64_t udp_datagrams;
    uint64_t icmp_messages;
} NetworkStats;

// Network stack functions
int netstack_init(void);
void netstack_cleanup(void);

// Socket operations
int netstack_socket(int family, int type, int protocol);
int netstack_bind(int sockfd, const SocketAddress* addr);
int netstack_listen(int sockfd, int backlog);
int netstack_accept(int sockfd, SocketAddress* addr);
int netstack_connect(int sockfd, const SocketAddress* addr);
int netstack_send(int sockfd, const void* data, uint32_t size, int flags);
int netstack_recv(int sockfd, void* data, uint32_t size, int flags);
int netstack_sendto(int sockfd, const void* data, uint32_t size, const SocketAddress* addr, int flags);
int netstack_recvfrom(int sockfd, void* data, uint32_t size, SocketAddress* addr, int flags);
int netstack_close(int sockfd);

// Interface management
int netstack_add_interface(const char* name, const IPv4Address* ip, const IPv4Address* netmask);
NetworkInterface* netstack_get_interface(int index);
NetworkInterface* netstack_find_interface(const char* name);
int netstack_interface_up(const char* name);
int netstack_interface_down(const char* name);

// Routing
int netstack_add_route(const IPv4Address* dest, const IPv4Address* mask, const IPv4Address* gateway, int metric);
int netstack_del_route(const IPv4Address* dest);
RouteEntry* netstack_find_route(const IPv4Address* dest);
void netstack_show_routes(void);

// Packet handling
int netstack_send_packet(const void* packet, uint32_t size, const IPv4Address* dest);
int netstack_receive_packet(void* packet, uint32_t max_size);

// Protocol handlers
int netstack_tcp_handshake(int sockfd);
int netstack_tcp_send_syn(int sockfd);
int netstack_tcp_send_ack(int sockfd);
int netstack_tcp_send_fin(int sockfd);
int netstack_udp_sendto(int sockfd, const void* data, uint32_t size, const SocketAddress* addr);
int netstack_icmp_ping(const IPv4Address* dest, uint16_t id, uint16_t seq);

// Utilities
uint16_t netstack_checksum(const void* data, uint32_t size);
int netstack_parse_ipv4(const char* str, IPv4Address* addr);
void netstack_format_ipv4(const IPv4Address* addr, char* str, uint32_t size);
int netstack_parse_mac(const char* str, MACAddress* mac);
void netstack_format_mac(const MACAddress* mac, char* str, uint32_t size);

// Statistics
NetworkStats* netstack_get_stats(void);
void netstack_dump_stats(void);
void netstack_show_connections(void);

#endif // KERNEL_NETWORK_STACK_H
