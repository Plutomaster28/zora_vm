#ifndef NETWORK_ADVANCED_H
#define NETWORK_ADVANCED_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// Maximum limits
#define MAX_INTERFACES 16
#define MAX_ROUTES 64
#define MAX_CONNECTIONS 256
#define MAX_VPN_TUNNELS 8
#define MAX_FIREWALL_RULES 128
#define MAX_PACKET_CAPTURE 1000

// Network interface types
typedef enum {
    IFACE_ETHERNET,
    IFACE_LOOPBACK,
    IFACE_VPN,
    IFACE_TUNNEL,
    IFACE_BRIDGE,
    IFACE_TAP,
    IFACE_TUN
} InterfaceType;

// Network protocols
typedef enum {
    PROTO_TCP = 6,
    PROTO_UDP = 17,
    PROTO_ICMP = 1,
    PROTO_GRE = 47,
    PROTO_ESP = 50
} NetworkProtocol;

// VPN types
typedef enum {
    VPN_NONE,
    VPN_OPENVPN,
    VPN_IPSEC,
    VPN_WIREGUARD,
    VPN_L2TP,
    VPN_PPTP
} VPNType;

// Firewall actions
typedef enum {
    FW_ACCEPT,
    FW_DROP,
    FW_REJECT,
    FW_LOG,
    FW_REDIRECT
} FirewallAction;

// Network interface structure
typedef struct {
    char name[16];              // Interface name (eth0, wlan0, etc.)
    InterfaceType type;
    char ip_address[16];        // IPv4 address
    char netmask[16];           // Subnet mask
    char gateway[16];           // Gateway IP
    char mac_address[18];       // MAC address
    int mtu;                    // Maximum transmission unit
    int is_up;                  // Interface state
    uint64_t rx_bytes;          // Received bytes
    uint64_t tx_bytes;          // Transmitted bytes
    uint64_t rx_packets;        // Received packets
    uint64_t tx_packets;        // Transmitted packets
    uint32_t rx_errors;         // Receive errors
    uint32_t tx_errors;         // Transmit errors
} NetworkInterface;

// Routing table entry
typedef struct {
    char destination[16];       // Destination network
    char gateway[16];           // Gateway IP
    char netmask[16];           // Network mask
    char iface[16];             // Output interface
    int metric;                 // Route metric
    int is_default;             // Default route flag
} RouteEntry;

// Active network connection
typedef struct {
    NetworkProtocol protocol;
    char local_ip[16];
    int local_port;
    char remote_ip[16];
    int remote_port;
    char state[16];             // ESTABLISHED, LISTEN, etc.
    time_t established_time;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} NetworkConnection;

// VPN tunnel configuration
typedef struct {
    char name[32];              // Tunnel name
    VPNType type;               // VPN protocol type
    char local_ip[16];          // Local endpoint
    char remote_ip[16];         // Remote endpoint
    char tunnel_ip[16];         // Tunnel interface IP
    char preshared_key[64];     // Pre-shared key
    int port;                   // VPN port
    int is_connected;           // Connection status
    time_t connected_since;     // Connection timestamp
    uint64_t tunnel_rx_bytes;   // Tunnel RX bytes
    uint64_t tunnel_tx_bytes;   // Tunnel TX bytes
} VPNTunnel;

// Firewall rule
typedef struct {
    int rule_id;                // Rule identifier
    char source_ip[16];         // Source IP (or 0.0.0.0 for any)
    char dest_ip[16];           // Destination IP
    int source_port;            // Source port (0 for any)
    int dest_port;              // Destination port
    NetworkProtocol protocol;   // Protocol
    FirewallAction action;      // Action to take
    char iface[16];             // Interface (or * for any)
    int packet_count;           // Matched packets
    uint64_t byte_count;        // Matched bytes
    time_t last_match;          // Last match time
    char description[64];       // Rule description
} FirewallRule;

// Packet capture entry
typedef struct {
    time_t timestamp;
    char source_ip[16];
    char dest_ip[16];
    int source_port;
    int dest_port;
    NetworkProtocol protocol;
    int packet_size;
    char iface[16];
    char payload_preview[64];   // First bytes of payload
} PacketCapture;

// QoS (Quality of Service) configuration
typedef struct {
    char class_name[32];        // Traffic class name
    int priority;               // Priority (0-7)
    int bandwidth_limit;        // Bandwidth limit in Kbps
    int burst_size;             // Burst size in bytes
    char match_criteria[64];    // Match criteria (port, protocol, etc.)
} QoSRule;

// Network statistics
typedef struct {
    uint64_t total_packets_rx;
    uint64_t total_packets_tx;
    uint64_t total_bytes_rx;
    uint64_t total_bytes_tx;
    uint32_t total_connections;
    uint32_t active_connections;
    uint32_t dropped_packets;
    uint32_t retransmissions;
    double packet_loss_rate;
    double average_latency;
} NetworkStats;

// Main network state structure
typedef struct {
    // Interfaces
    NetworkInterface interfaces[MAX_INTERFACES];
    int interface_count;
    
    // Routing
    RouteEntry routes[MAX_ROUTES];
    int route_count;
    
    // Active connections
    NetworkConnection connections[MAX_CONNECTIONS];
    int connection_count;
    
    // VPN tunnels
    VPNTunnel vpn_tunnels[MAX_VPN_TUNNELS];
    int vpn_count;
    
    // Firewall
    FirewallRule firewall_rules[MAX_FIREWALL_RULES];
    int firewall_rule_count;
    int firewall_enabled;
    
    // Packet capture
    PacketCapture packet_captures[MAX_PACKET_CAPTURE];
    int capture_count;
    int capture_enabled;
    
    // QoS
    QoSRule qos_rules[32];
    int qos_rule_count;
    int qos_enabled;
    
    // Statistics
    NetworkStats stats;
    
    // DNS configuration
    char dns_servers[4][16];    // Up to 4 DNS servers
    int dns_server_count;
    
    // Network security
    int allow_forwarding;       // IP forwarding
    int allow_source_route;     // Source routing
    int syn_flood_protection;   // SYN flood protection
    int icmp_redirect_accept;   // ICMP redirects
    
    // Advanced features
    int nat_enabled;            // NAT/masquerading
    int bridge_mode;            // Bridge mode
    int promiscuous_mode;       // Promiscuous mode
    
    // Network namespace simulation
    char namespace_name[32];    // Network namespace
    int isolated_namespace;     // Namespace isolation
} AdvancedNetworkState;

// Core functions
int advanced_network_init(void);
void advanced_network_cleanup(void);

// Interface management
int network_add_interface(const char* name, InterfaceType type);
int network_remove_interface(const char* name);
int network_configure_interface(const char* name, const char* ip, const char* netmask, const char* gateway);
int network_set_interface_state(const char* name, int up);
int network_set_interface_mtu(const char* name, int mtu);
NetworkInterface* network_get_interface(const char* name);
void network_show_interface_details(const char* name);

// Routing functions
int network_add_route(const char* dest, const char* gateway, const char* netmask, const char* iface, int metric);
int network_remove_route(const char* dest, const char* gateway);
int network_set_default_route(const char* gateway, const char* iface);
void network_show_routing_table(void);
RouteEntry* network_find_route(const char* destination);

// Real socket operations
int network_create_socket(NetworkProtocol protocol);
int network_bind_socket(int socket_fd, const char* ip, int port);
int network_listen_socket(int socket_fd, int backlog);
int network_accept_connection(int socket_fd);
int network_connect_socket(int socket_fd, const char* host, int port);
int network_send_data(int socket_fd, const void* data, size_t length);
int network_receive_data(int socket_fd, void* buffer, size_t buffer_size);
void network_close_socket(int socket_fd);

// Advanced networking
int network_create_raw_socket(void);
int network_send_raw_packet(int socket_fd, const void* packet, size_t length);
int network_capture_packets(const char* iface, int count);

// VPN and tunneling
int network_create_vpn_tunnel(const char* name, VPNType type, const char* remote_ip, int port);
int network_connect_vpn_tunnel(const char* name, const char* username, const char* password);
int network_disconnect_vpn_tunnel(const char* name);
void network_show_vpn_status(void);
int network_route_through_tunnel(const char* tunnel_name, const char* destination);

// Firewall management
int network_add_firewall_rule(const FirewallRule* rule);
int network_remove_firewall_rule(int rule_id);
int network_enable_firewall(void);
int network_disable_firewall(void);
void network_show_firewall_rules(void);
int network_check_firewall_packet(const char* source_ip, const char* dest_ip, int source_port, int dest_port, NetworkProtocol protocol);

// Quality of Service
int network_add_qos_rule(const QoSRule* rule);
int network_remove_qos_rule(const char* class_name);
int network_enable_qos(void);
int network_disable_qos(void);
void network_show_qos_rules(void);

// Network monitoring
void network_show_connections(void);
void network_show_listening_ports(void);
void network_show_statistics(void);
void network_show_packet_captures(void);
void network_clear_statistics(void);

// Network utilities
int network_ping_host(const char* hostname, int count, int timeout);
int network_traceroute(const char* hostname, int max_hops);
int network_port_scan(const char* hostname, int start_port, int end_port);
int network_bandwidth_test(const char* hostname, int port, int duration);

// DNS functions
int network_resolve_hostname(const char* hostname, char* ip_result, size_t ip_size);
int network_reverse_dns_lookup(const char* ip, char* hostname_result, size_t hostname_size);
int network_add_dns_server(const char* dns_ip);
int network_remove_dns_server(const char* dns_ip);

// Network security
int network_enable_syn_flood_protection(void);
int network_disable_syn_flood_protection(void);
int network_block_ip_address(const char* ip);
int network_unblock_ip_address(const char* ip);
int network_detect_port_scan(void);

// Advanced features
int network_enable_nat(const char* internal_iface, const char* external_iface);
int network_disable_nat(void);
int network_create_bridge(const char* bridge_name);
int network_add_interface_to_bridge(const char* bridge_name, const char* iface_name);
int network_enable_promiscuous_mode(const char* iface);
int network_disable_promiscuous_mode(const char* iface);

// Network namespace simulation
int network_create_namespace(const char* namespace_name);
int network_switch_namespace(const char* namespace_name);
int network_delete_namespace(const char* namespace_name);
void network_show_namespaces(void);

#endif // NETWORK_ADVANCED_H