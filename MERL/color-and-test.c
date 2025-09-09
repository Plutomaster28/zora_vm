#include <stdio.h>
#include <time.h>
#include "color-and-test.h"
#include "config.h"

void color_and_test_command(int argc, char **argv) {
    // ASCII art for Zora VM
    printf("\033[36m"); // Cyan color
    printf("                 ████████\n");
    printf("               ██        ██\n");
    printf("             ██    ████    ██\n");
    printf("           ██    ██    ██    ██\n");
    printf("         ██    ██        ██    ██\n");
    printf("       ██    ██            ██    ██\n");
    printf("     ██    ██                ██    ██\n");
    printf("   ██    ██      \033[31mZORA VM\033[36m      ██    ██\n");
    printf("     ██    ██                ██    ██\n");
    printf("       ██    ██            ██    ██\n");
    printf("         ██    ██        ██    ██\n");
    printf("           ██    ██    ██    ██\n");
    printf("             ██    ████    ██\n");
    printf("               ██        ██\n");
    printf("                 ████████\n");
    printf("\033[0m"); // Reset color

    printf("\n");
    
    // System information in neofetch style
    printf("\033[1;32m%s\033[0m@\033[1;32m%s\033[0m\n", "guest", "zora-vm");
    printf("\033[1;34m─────────────────────────\033[0m\n");
    
    printf("\033[1;31mOS\033[0m:         %s\n", OS_VERSION);
    printf("\033[1;32mKernel\033[0m:     Virtual MERL\n");
    printf("\033[1;33mUptime\033[0m:     ");
    
    // Calculate uptime (simulated)
    static time_t start_time = 0;
    if (start_time == 0) start_time = time(NULL);
    time_t uptime = time(NULL) - start_time;
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;
    
    if (hours > 0) printf("%dh ", hours);
    if (minutes > 0) printf("%dm ", minutes);
    printf("%ds\n", seconds);
    
    printf("\033[1;34mShell\033[0m:      MERL Shell\n");
    printf("\033[1;35mResolution\033[0m: Terminal-based\n");
    printf("\033[1;36mInterface\033[0m:  Campbell Color Scheme\n");
    printf("\033[1;31mWM\033[0m:         Virtual Window Manager\n");
    printf("\033[1;32mTerminal\033[0m:   MERL Terminal\n");
    printf("\033[1;33mCPU\033[0m:        Virtual CPU (40 threads)\n");
    printf("\033[1;34mGPU\033[0m:        Meisei Virtual Silicon\n");
    printf("\033[1;35mMemory\033[0m:     86M / %d MB\n", TOTAL_MEMORY_MB);
    printf("\033[1;36mFirmware\033[0m:   %s\n", FIRMWARE_VERSION);
    printf("\033[1;31mBIOS\033[0m:       %s\n", BIOS);
    
    printf("\n");
    
    // Color palette showcase
    printf("\033[1;37mColors:\033[0m ");
    
    // Normal colors
    for (int i = 0; i < 8; i++) {
        printf("\033[%dm██\033[0m", 40 + i);
    }
    printf(" ");
    
    // Bright colors  
    for (int i = 0; i < 8; i++) {
        printf("\033[%dm██\033[0m", 100 + i);
    }
    printf("\n");
    
    printf("\n");
    
    // Zora VM branding
    printf("\033[1;36m╭─────────────────────────────────────╮\033[0m\n");
    printf("\033[1;36m│\033[0m  \033[1;35m Powered by Zora Virtual Machine\033[0m  \033[1;36m│\033[0m\n");
    printf("\033[1;36m│\033[0m     \033[1;33mAdvanced VM with VFS & Sandboxing\033[0m   \033[1;36m│\033[0m\n");
    printf("\033[1;36m╰─────────────────────────────────────╯\033[0m\n");
}