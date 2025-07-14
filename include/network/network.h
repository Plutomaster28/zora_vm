// Create include/network/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

// Virtual network interface
typedef struct {
    char name[16];              // Interface name (e.g., "veth0")
    uint32_t ip_address;        // IP address (network order)
    uint32_t netmask;           // Subnet mask
    uint32_t gateway;           // Default gateway
    int is_up;                  // Interface state
    uint64_t rx_bytes;          // Received bytes
    uint64_t tx_bytes;          // Transmitted bytes
    uint64_t rx_packets;        // Received packets
    uint64_t tx_packets;        // Transmitted packets
} VirtualNetInterface;

// Virtual connection
typedef struct VirtualConnection {
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    int protocol;               // TCP=1, UDP=2
    int state;                  // Connected, listening, etc.
    struct VirtualConnection* next;
} VirtualConnection;

// Virtual network context
typedef struct {
    VirtualNetInterface interfaces[4];  // Up to 4 virtual interfaces
    int interface_count;
    VirtualConnection* connections;
    char dns_servers[3][16];            // Up to 3 DNS servers
    int dns_count;
    int firewall_enabled;
} VirtualNetwork;

// Network functions
int network_init(void);
void network_cleanup(void);
void network_show_interfaces(void);
int network_interface_up(const char* iface_name);
int network_interface_down(const char* iface_name);
int network_set_ip(const char* iface_name, const char* ip, const char* netmask);
void network_simulate_ping(const char* target);
void network_show_connections(void);
void network_show_routes(void);
int network_simulate_connect(const char* host, int port, int protocol);

#endif // NETWORK_H