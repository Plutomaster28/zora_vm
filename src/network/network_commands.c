#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "network/network_advanced.h"

// Enhanced networking commands for ZoraVM

void advanced_ifconfig_command(int argc, char **argv) {
    if (argc == 1) {
        // Show all interfaces
        printf("Network Interface Configuration:\n");
        printf("═══════════════════════════════════════════════════════════\n");
        
        for (int i = 0; i < 16; i++) {  // MAX_INTERFACES
            char iface_name[16];
            sprintf(iface_name, "eth%d", i);
            NetworkInterface* iface = network_get_interface(iface_name);
            if (iface) {
                network_show_interface_details(iface_name);
                printf("\n");
            }
            
            if (i == 0) {
                // Always show loopback
                network_show_interface_details("lo");
                printf("\n");
            }
        }
        return;
    }
    
    if (argc >= 2) {
        char* interface = argv[1];
        
        if (argc == 2) {
            // Show specific interface
            network_show_interface_details(interface);
            return;
        }
        
        // Configure interface
        if (argc >= 4 && strcmp(argv[2], "inet") == 0) {
            char* ip = argv[3];
            char* netmask = "255.255.255.0";
            
            if (argc >= 6 && strcmp(argv[4], "netmask") == 0) {
                netmask = argv[5];
            }
            
            network_configure_interface(interface, ip, netmask, "0.0.0.0");
            printf("Interface %s configured with IP %s/%s\n", interface, ip, netmask);
        }
        else if (argc >= 3 && strcmp(argv[2], "up") == 0) {
            network_set_interface_state(interface, 1);
        }
        else if (argc >= 3 && strcmp(argv[2], "down") == 0) {
            network_set_interface_state(interface, 0);
        }
        else {
            printf("Usage: ifconfig [interface] [inet IP] [netmask MASK] [up|down]\n");
            printf("       ifconfig interface up|down\n");
            printf("       ifconfig interface inet IP netmask MASK\n");
        }
    }
}

void advanced_ping_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ping [-c count] [-t timeout] hostname\n");
        printf("Options:\n");
        printf("  -c count    Number of packets to send (default: 4)\n");
        printf("  -t timeout  Timeout in milliseconds (default: 5000)\n");
        return;
    }
    
    char* hostname = NULL;
    int count = 4;
    int timeout = 5000;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            count = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        }
        else if (!hostname) {
            hostname = argv[i];
        }
    }
    
    if (!hostname) {
        printf("Error: hostname required\n");
        return;
    }
    
    network_ping_host(hostname, count, timeout);
}

void advanced_netstat_command(int argc, char **argv) {
    int show_listening = 0;
    int show_routes = 0;
    int show_interfaces = 0;
    int show_statistics = 0;
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--listening") == 0) {
            show_listening = 1;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--route") == 0) {
            show_routes = 1;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interfaces") == 0) {
            show_interfaces = 1;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--statistics") == 0) {
            show_statistics = 1;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: netstat [options]\n");
            printf("Options:\n");
            printf("  -l, --listening    Show only listening ports\n");
            printf("  -r, --route        Display routing table\n");
            printf("  -i, --interfaces   Display network interfaces\n");
            printf("  -s, --statistics   Display network statistics\n");
            printf("  -h, --help         Show this help message\n");
            return;
        }
    }
    
    if (show_routes) {
        network_show_routing_table();
        printf("\n");
    }
    
    if (show_interfaces) {
        advanced_ifconfig_command(1, NULL);
        printf("\n");
    }
    
    if (show_statistics) {
        network_show_statistics();
        printf("\n");
    }
    
    if (show_listening) {
        network_show_listening_ports();
        printf("\n");
    }
    
    if (!show_listening && !show_routes && !show_interfaces && !show_statistics) {
        // Default: show active connections
        network_show_connections();
    }
}

void advanced_route_command(int argc, char **argv) {
    if (argc == 1) {
        network_show_routing_table();
        return;
    }
    
    if (strcmp(argv[1], "add") == 0) {
        if (argc < 5) {
            printf("Usage: route add <destination> <gateway> [netmask] [interface]\n");
            printf("       route add default <gateway> [interface]\n");
            return;
        }
        
        char* destination = argv[2];
        char* gateway = argv[3];
        char* netmask = (argc > 4) ? argv[4] : "255.255.255.0";
        char* interface = (argc > 5) ? argv[5] : "eth0";
        
        if (strcmp(destination, "default") == 0) {
            network_set_default_route(gateway, interface);
        } else {
            network_add_route(destination, gateway, netmask, interface, 100);
        }
    }
    else if (strcmp(argv[1], "del") == 0 || strcmp(argv[1], "delete") == 0) {
        if (argc < 4) {
            printf("Usage: route del <destination> <gateway>\n");
            return;
        }
        
        char* destination = argv[2];
        char* gateway = argv[3];
        
        network_remove_route(destination, gateway);
    }
    else if (strcmp(argv[1], "show") == 0) {
        network_show_routing_table();
    }
    else {
        printf("Usage: route [add|del|show] ...\n");
        printf("Commands:\n");
        printf("  add <dest> <gateway> [netmask] [iface]  Add route\n");
        printf("  del <dest> <gateway>                    Delete route\n");
        printf("  show                                    Show routing table\n");
    }
}

void firewall_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: firewall [enable|disable|list|add|remove] ...\n");
        printf("Commands:\n");
        printf("  enable                          Enable firewall\n");
        printf("  disable                         Disable firewall\n");
        printf("  list                            List all rules\n");
        printf("  add <rule_spec>                 Add firewall rule\n");
        printf("  remove <rule_id>                Remove rule by ID\n");
        printf("\nRule specification format:\n");
        printf("  --source IP --dest IP --sport PORT --dport PORT --proto PROTO --action ACTION\n");
        printf("  Example: firewall add --dest 0.0.0.0 --dport 80 --proto tcp --action accept\n");
        return;
    }
    
    if (strcmp(argv[1], "enable") == 0) {
        network_enable_firewall();
    }
    else if (strcmp(argv[1], "disable") == 0) {
        network_disable_firewall();
    }
    else if (strcmp(argv[1], "list") == 0) {
        network_show_firewall_rules();
    }
    else if (strcmp(argv[1], "add") == 0) {
        // Parse firewall rule
        FirewallRule rule = {0};
        rule.rule_id = rand() % 10000;
        strcpy(rule.source_ip, "0.0.0.0");
        strcpy(rule.dest_ip, "0.0.0.0");
        strcpy(rule.iface, "*");
        rule.protocol = PROTO_TCP;
        rule.action = FW_ACCEPT;
        
        for (int i = 2; i < argc - 1; i++) {
            if (strcmp(argv[i], "--source") == 0) {
                strncpy(rule.source_ip, argv[++i], 15);
            }
            else if (strcmp(argv[i], "--dest") == 0) {
                strncpy(rule.dest_ip, argv[++i], 15);
            }
            else if (strcmp(argv[i], "--sport") == 0) {
                rule.source_port = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "--dport") == 0) {
                rule.dest_port = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "--proto") == 0) {
                char* proto = argv[++i];
                if (strcmp(proto, "tcp") == 0) rule.protocol = PROTO_TCP;
                else if (strcmp(proto, "udp") == 0) rule.protocol = PROTO_UDP;
                else if (strcmp(proto, "icmp") == 0) rule.protocol = PROTO_ICMP;
            }
            else if (strcmp(argv[i], "--action") == 0) {
                char* action = argv[++i];
                if (strcmp(action, "accept") == 0) rule.action = FW_ACCEPT;
                else if (strcmp(action, "drop") == 0) rule.action = FW_DROP;
                else if (strcmp(action, "reject") == 0) rule.action = FW_REJECT;
            }
            else if (strcmp(argv[i], "--desc") == 0) {
                strncpy(rule.description, argv[++i], 63);
            }
        }
        
        if (strlen(rule.description) == 0) {
            snprintf(rule.description, 63, "User rule %d", rule.rule_id);
        }
        
        network_add_firewall_rule(&rule);
    }
    else if (strcmp(argv[1], "remove") == 0) {
        if (argc < 3) {
            printf("Usage: firewall remove <rule_id>\n");
            return;
        }
        
        int rule_id = atoi(argv[2]);
        network_remove_firewall_rule(rule_id);
    }
}

void vpn_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: vpn [create|connect|disconnect|status|list] ...\n");
        printf("Commands:\n");
        printf("  create <name> <type> <remote_ip> <port>  Create VPN tunnel\n");
        printf("  connect <name> [username] [password]     Connect to VPN\n");
        printf("  disconnect <name>                        Disconnect VPN\n");
        printf("  status [name]                            Show VPN status\n");
        printf("  list                                     List all VPN tunnels\n");
        printf("\nVPN Types: openvpn, ipsec, wireguard, l2tp, pptp\n");
        return;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        if (argc < 6) {
            printf("Usage: vpn create <name> <type> <remote_ip> <port>\n");
            return;
        }
        
        char* name = argv[2];
        char* type_str = argv[3];
        char* remote_ip = argv[4];
        int port = atoi(argv[5]);
        
        VPNType type = VPN_OPENVPN;
        if (strcmp(type_str, "ipsec") == 0) type = VPN_IPSEC;
        else if (strcmp(type_str, "wireguard") == 0) type = VPN_WIREGUARD;
        else if (strcmp(type_str, "l2tp") == 0) type = VPN_L2TP;
        else if (strcmp(type_str, "pptp") == 0) type = VPN_PPTP;
        
        network_create_vpn_tunnel(name, type, remote_ip, port);
    }
    else if (strcmp(argv[1], "connect") == 0) {
        if (argc < 3) {
            printf("Usage: vpn connect <name> [username] [password]\n");
            return;
        }
        
        char* name = argv[2];
        char* username = (argc > 3) ? argv[3] : "user";
        char* password = (argc > 4) ? argv[4] : "password";
        
        network_connect_vpn_tunnel(name, username, password);
    }
    else if (strcmp(argv[1], "disconnect") == 0) {
        if (argc < 3) {
            printf("Usage: vpn disconnect <name>\n");
            return;
        }
        
        char* name = argv[2];
        network_disconnect_vpn_tunnel(name);
    }
    else if (strcmp(argv[1], "status") == 0) {
        network_show_vpn_status();
    }
    else if (strcmp(argv[1], "list") == 0) {
        network_show_vpn_status();
    }
}

void traceroute_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: traceroute [-m max_hops] hostname\n");
        printf("Options:\n");
        printf("  -m max_hops    Maximum number of hops (default: 30)\n");
        return;
    }
    
    char* hostname = NULL;
    int max_hops = 30;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            max_hops = atoi(argv[++i]);
        }
        else if (!hostname) {
            hostname = argv[i];
        }
    }
    
    if (!hostname) {
        printf("Error: hostname required\n");
        return;
    }
    
    network_traceroute(hostname, max_hops);
}

void portscan_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: portscan [-p start-end] hostname\n");
        printf("Options:\n");
        printf("  -p start-end   Port range to scan (default: 1-1024)\n");
        printf("Examples:\n");
        printf("  portscan google.com\n");
        printf("  portscan -p 80-443 example.com\n");
        return;
    }
    
    char* hostname = NULL;
    int start_port = 1;
    int end_port = 1024;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            char* range = argv[++i];
            sscanf(range, "%d-%d", &start_port, &end_port);
        }
        else if (!hostname) {
            hostname = argv[i];
        }
    }
    
    if (!hostname) {
        printf("Error: hostname required\n");
        return;
    }
    
    network_port_scan(hostname, start_port, end_port);
}

void bandwidth_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: bandwidth [-p port] [-t duration] hostname\n");
        printf("Options:\n");
        printf("  -p port       Port to test (default: 80)\n");
        printf("  -t duration   Test duration in seconds (default: 10)\n");
        return;
    }
    
    char* hostname = NULL;
    int port = 80;
    int duration = 10;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            duration = atoi(argv[++i]);
        }
        else if (!hostname) {
            hostname = argv[i];
        }
    }
    
    if (!hostname) {
        printf("Error: hostname required\n");
        return;
    }
    
    network_bandwidth_test(hostname, port, duration);
}

void netns_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: netns [create|switch|delete|list] ...\n");
        printf("Commands:\n");
        printf("  create <name>    Create network namespace\n");
        printf("  switch <name>    Switch to namespace\n");
        printf("  delete <name>    Delete namespace\n");
        printf("  list             List all namespaces\n");
        return;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        if (argc < 3) {
            printf("Usage: netns create <name>\n");
            return;
        }
        network_create_namespace(argv[2]);
    }
    else if (strcmp(argv[1], "switch") == 0) {
        if (argc < 3) {
            printf("Usage: netns switch <name>\n");
            return;
        }
        network_switch_namespace(argv[2]);
    }
    else if (strcmp(argv[1], "delete") == 0) {
        if (argc < 3) {
            printf("Usage: netns delete <name>\n");
            return;
        }
        network_delete_namespace(argv[2]);
    }
    else if (strcmp(argv[1], "list") == 0) {
        network_show_namespaces();
    }
}