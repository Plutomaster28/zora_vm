// Create include/network/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

// Virtual Network State
typedef struct {
    char vm_ip[16];           // VM's virtual IP (e.g., 10.0.2.15)
    char gateway_ip[16];      // Virtual gateway (e.g., 10.0.2.1)
    char subnet_mask[16];     // Subnet mask (255.255.255.0)
    char dns_server[16];      // Virtual DNS server
    int virtual_interface_up;
    int nat_enabled;
    int dhcp_enabled;
    
    // Security settings
    int allow_outbound;
    int allow_http;
    int allow_https;
    int allow_dns;
    int block_dangerous_ports;
} VirtualNetwork;

// Core network functions
int network_init(void);
void network_cleanup(void);

// Interface management
void network_show_interfaces(void);
int network_interface_up(const char* iface_name);
int network_interface_down(const char* iface_name);
int network_set_ip(const char* iface_name, const char* ip, const char* netmask);

// Network operations
int network_resolve_dns(const char* hostname, char* ip_result, size_t ip_size);
int network_http_request(const char* url, const char* method);
void network_simulate_ping(const char* target);
int network_simulate_connect(const char* host, int port, int protocol);

// Network information
void network_show_connections(void);
void network_show_routes(void);
void network_show_diagnostics(void);

// Security
void network_set_security_policy(int allow_http, int allow_https, int allow_dns, int block_dangerous);
int network_is_port_allowed(int port);

#endif // NETWORK_H