// Create src/network/network.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network/network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

// Remove the duplicate VirtualNetwork struct - it's already in the header!

static VirtualNetwork* vnet = NULL;

// Initialize virtual network with VMware-style configuration
int network_init(void) {
    printf("Initializing Zora VM Virtual Network...\n");
    
    if (vnet) {
        return 0; // Already initialized
    }
    
    vnet = calloc(1, sizeof(VirtualNetwork));
    if (!vnet) {
        printf("Failed to allocate virtual network\n");
        return -1;
    }
    
    // Default VMware-style configuration
    strcpy(vnet->vm_ip, "10.0.2.15");          // VM IP
    strcpy(vnet->gateway_ip, "10.0.2.1");      // Virtual gateway
    strcpy(vnet->subnet_mask, "255.255.255.0"); // Standard subnet
    strcpy(vnet->dns_server, "10.0.2.3");      // Virtual DNS
    
    vnet->virtual_interface_up = 1;
    vnet->nat_enabled = 1;
    vnet->dhcp_enabled = 1;
    
    // Security defaults (safe configuration)
    vnet->allow_outbound = 1;
    vnet->allow_http = 1;
    vnet->allow_https = 1;
    vnet->allow_dns = 1;
    vnet->block_dangerous_ports = 1;
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        free(vnet);
        vnet = NULL;
        return -1;
    }
    
    printf("Virtual Network initialized successfully\n");
    printf("VM Network Configuration:\n");
    printf("   IP Address:    %s\n", vnet->vm_ip);
    printf("   Gateway:       %s\n", vnet->gateway_ip);
    printf("   Subnet Mask:   %s\n", vnet->subnet_mask);
    printf("   DNS Server:    %s\n", vnet->dns_server);
    printf("   NAT:           %s\n", vnet->nat_enabled ? "ENABLED" : "DISABLED");
    printf("   Security:      ENABLED (Safe Mode)\n");
    
    return 0;
}

void network_cleanup(void) {
    if (!vnet) {
        return;
    }
    
    printf("Cleaning up virtual network...\n");
    
    WSACleanup();
    
    free(vnet);
    vnet = NULL;
    
    printf("Virtual network cleaned up\n");
}

// Enhanced interface display with real virtual network info
void network_show_interfaces(void) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return;
    }
    
    printf("Virtual Network Interfaces:\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("zora0     Link encap:Ethernet  HWaddr 08:00:27:12:34:56\n");
    printf("          inet addr:%s  Bcast:%s  Mask:%s\n", 
           vnet->vm_ip, "10.0.2.255", vnet->subnet_mask);
    printf("          %s BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1\n",
           vnet->virtual_interface_up ? "UP" : "DOWN");
    printf("          NAT: %s  DHCP: %s\n",
           vnet->nat_enabled ? "ENABLED" : "DISABLED",
           vnet->dhcp_enabled ? "ENABLED" : "DISABLED");
    printf("\n");
    printf("lo        Link encap:Local Loopback\n");
    printf("          inet addr:127.0.0.1  Mask:255.0.0.0\n");
    printf("          UP LOOPBACK RUNNING  MTU:65536  Metric:1\n");
    printf("══════════════════════════════════════════════════════════════\n");
}

// Real DNS resolution through virtual DNS
int network_resolve_dns(const char* hostname, char* ip_result, size_t ip_size) {
    if (!vnet || !vnet->allow_dns) {
        printf("DNS resolution blocked by security policy\n");
        return -1;
    }
    
    printf("Resolving %s through virtual DNS (%s)...\n", hostname, vnet->dns_server);
    
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        printf("DNS resolution failed: %s\n", gai_strerror(status));
        return -1;
    }
    
    struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
    strncpy(ip_result, inet_ntoa(addr_in->sin_addr), ip_size - 1);
    ip_result[ip_size - 1] = '\0';
    
    printf("%s resolved to %s\n", hostname, ip_result);
    
    freeaddrinfo(result);
    return 0;
}

// Secure HTTP/HTTPS requests through virtual NAT
int network_http_request(const char* url, const char* method) {
    if (!vnet || !vnet->nat_enabled) {
        printf("Network access disabled\n");
        return -1;
    }
    
    // Check if HTTP/HTTPS is allowed
    int is_https = (strncmp(url, "https://", 8) == 0);
    int is_http = (strncmp(url, "http://", 7) == 0);
    
    if (is_https && !vnet->allow_https) {
        printf("HTTPS requests blocked by security policy\n");
        return -1;
    }
    
    if (is_http && !vnet->allow_http) {
        printf("HTTP requests blocked by security policy\n");
        return -1;
    }
    
    if (!is_http && !is_https) {
        printf("Only HTTP/HTTPS requests are allowed\n");
        return -1;
    }
    
    printf("Virtual NAT: %s request to %s\n", method, url);
    printf("Request routed through secure virtual gateway (%s)\n", vnet->gateway_ip);
    
    // TODO: Implement actual HTTP client with security restrictions
    printf("[SIMULATED] Response: 200 OK\n");
    printf("[SIMULATED] Content: Virtual response data\n");
    
    return 0;
}

// Secure ping through virtual network
void network_simulate_ping(const char* target) {
    if (!vnet || !vnet->virtual_interface_up) {
        printf("Virtual network interface is down\n");
        return;
    }
    
    char resolved_ip[16];
    if (network_resolve_dns(target, resolved_ip, sizeof(resolved_ip)) != 0) {
        printf("Cannot ping %s: DNS resolution failed\n", target);
        return;
    }
    
    printf("PING %s (%s) from virtual interface %s\n", target, resolved_ip, vnet->vm_ip);
    printf("64 bytes from %s (%s): icmp_seq=1 ttl=64 time=0.123 ms\n", target, resolved_ip);
    printf("64 bytes from %s (%s): icmp_seq=2 ttl=64 time=0.089 ms\n", target, resolved_ip);
    printf("64 bytes from %s (%s): icmp_seq=3 ttl=64 time=0.102 ms\n", target, resolved_ip);
    printf("\n--- %s ping statistics ---\n", target);
    printf("3 packets transmitted, 3 received, 0%% packet loss, time 2003ms\n");
    printf("rtt min/avg/max/mdev = 0.089/0.105/0.123/0.014 ms\n");
}

// Network security configuration
void network_set_security_policy(int allow_http, int allow_https, int allow_dns, int block_dangerous) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return;
    }
    
    vnet->allow_http = allow_http;
    vnet->allow_https = allow_https;
    vnet->allow_dns = allow_dns;
    vnet->block_dangerous_ports = block_dangerous;
    
    printf("Virtual Network Security Policy Updated:\n");
    printf("   HTTP:              %s\n", allow_http ? "ALLOWED" : "BLOCKED");
    printf("   HTTPS:             %s\n", allow_https ? "ALLOWED" : "BLOCKED");
    printf("   DNS:               %s\n", allow_dns ? "ALLOWED" : "BLOCKED");
    printf("   Block Dangerous:   %s\n", block_dangerous ? "ENABLED" : "DISABLED");
}

// Show virtual network connections
void network_show_connections(void) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return;
    }
    
    printf("Active Virtual Network Connections:\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("Proto Recv-Q Send-Q Local Address           Foreign Address         State\n");
    printf("tcp        0      0 %s:22       %s:54321       ESTABLISHED\n", vnet->vm_ip, vnet->gateway_ip);
    printf("tcp        0      0 %s:80       8.8.8.8:443            TIME_WAIT\n", vnet->vm_ip);
    printf("udp        0      0 %s:53       %s:53          ESTABLISHED\n", vnet->vm_ip, vnet->dns_server);
    printf("══════════════════════════════════════════════════════════════\n");
}

// Virtual routing table
void network_show_routes(void) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return;
    }
    
    printf("Virtual Network Routing Table:\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface\n");
    printf("0.0.0.0         %s     0.0.0.0         UG        0 0          0 zora0\n", vnet->gateway_ip);
    printf("10.0.2.0        0.0.0.0         255.255.255.0   U         0 0          0 zora0\n");
    printf("127.0.0.0       0.0.0.0         255.0.0.0       U         0 0          0 lo\n");
    printf("══════════════════════════════════════════════════════════════\n");
}

// Port security check
int network_is_port_allowed(int port) {
    if (!vnet || !vnet->block_dangerous_ports) {
        return 1; // Allow all if security is disabled
    }
    
    // Block dangerous ports
    int dangerous_ports[] = {23, 135, 139, 445, 593, 1433, 1434, 3389, 5900, 6129};
    int num_dangerous = sizeof(dangerous_ports) / sizeof(dangerous_ports[0]);
    
    for (int i = 0; i < num_dangerous; i++) {
        if (port == dangerous_ports[i]) {
            printf("Port %d blocked by security policy (dangerous service)\n", port);
            return 0;
        }
    }
    
    // Allow common safe ports
    if (port == 80 || port == 443 || port == 53 || port == 22) {
        return 1;
    }
    
    // Block privileged ports for unprivileged access
    if (port < 1024) {
        printf("Port %d blocked (privileged port)\n", port);
        return 0;
    }
    
    return 1; // Allow other ports
}

// Enhanced connection simulation with security
int network_simulate_connect(const char* host, int port, int protocol) {
    if (!vnet || !vnet->virtual_interface_up) {
        printf("Virtual network interface is down\n");
        return -1;
    }
    
    if (!network_is_port_allowed(port)) {
        return -1;
    }
    
    char resolved_ip[16];
    if (network_resolve_dns(host, resolved_ip, sizeof(resolved_ip)) != 0) {
        return -1;
    }
    
    const char* proto_name = (protocol == 1) ? "TCP" : "UDP";
    printf("Virtual NAT: Attempting %s connection to %s:%d (%s)...\n", 
           proto_name, host, port, resolved_ip);
    printf("Connection routed through secure gateway %s\n", vnet->gateway_ip);
    printf("Connected to %s (%s) via virtual interface\n", host, resolved_ip);
    
    return 0;
}

// Network diagnostics
void network_show_diagnostics(void) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return;
    }
    
    printf("Virtual Network Diagnostics:\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("Virtual Interface: %s\n", vnet->virtual_interface_up ? "UP" : "DOWN");
    printf("NAT Gateway:       %s (%s)\n", vnet->nat_enabled ? "ENABLED" : "DISABLED", vnet->gateway_ip);
    printf("DHCP Service:      %s\n", vnet->dhcp_enabled ? "ENABLED" : "DISABLED");
    printf("DNS Service:       %s (%s)\n", vnet->allow_dns ? "ENABLED" : "DISABLED", vnet->dns_server);
    printf("Security Policy:   %s\n", vnet->block_dangerous_ports ? "STRICT" : "PERMISSIVE");
    printf("Outbound Traffic:  %s\n", vnet->allow_outbound ? "ALLOWED" : "BLOCKED");
    printf("HTTP Access:       %s\n", vnet->allow_http ? "ALLOWED" : "BLOCKED");
    printf("HTTPS Access:      %s\n", vnet->allow_https ? "ALLOWED" : "BLOCKED");
    printf("══════════════════════════════════════════════════════════════\n");
}

// Interface management functions
int network_interface_up(const char* iface_name) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return -1;
    }
    
    if (strcmp(iface_name, "zora0") == 0) {
        vnet->virtual_interface_up = 1;
        printf("Interface %s is now UP\n", iface_name);
        return 0;
    } else if (strcmp(iface_name, "lo") == 0) {
        printf("Loopback interface is always UP\n");
        return 0;
    }
    
    printf("Unknown interface: %s\n", iface_name);
    return -1;
}

int network_interface_down(const char* iface_name) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return -1;
    }
    
    if (strcmp(iface_name, "zora0") == 0) {
        vnet->virtual_interface_up = 0;
        printf("⬇Interface %s is now DOWN\n", iface_name);
        return 0;
    } else if (strcmp(iface_name, "lo") == 0) {
        printf("Cannot bring down loopback interface\n");
        return -1;
    }
    
    printf("Unknown interface: %s\n", iface_name);
    return -1;
}

int network_set_ip(const char* iface_name, const char* ip, const char* netmask) {
    if (!vnet) {
        printf("Virtual network not initialized\n");
        return -1;
    }
    
    if (strcmp(iface_name, "zora0") == 0) {
        strncpy(vnet->vm_ip, ip, sizeof(vnet->vm_ip) - 1);
        vnet->vm_ip[sizeof(vnet->vm_ip) - 1] = '\0';
        
        strncpy(vnet->subnet_mask, netmask, sizeof(vnet->subnet_mask) - 1);
        vnet->subnet_mask[sizeof(vnet->subnet_mask) - 1] = '\0';
        
        printf("Interface %s IP set to %s/%s\n", iface_name, ip, netmask);
        return 0;
    } else if (strcmp(iface_name, "lo") == 0) {
        printf("Cannot change loopback interface IP\n");
        return -1;
    }
    
    printf("Unknown interface: %s\n", iface_name);
    return -1;
}