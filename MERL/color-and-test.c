#include <stdio.h>
#include "color-and-test.h"
#include "config.h"

void color_and_test_command(int argc, char **argv) {
    // Print a simple ASCII logo
    printf("   __  __ _____ ____  _      \n");
    printf("  |  \\/  | ____|  _ \\| |     \n");
    printf("  | |\\/| |  _| | |_) | |     \n");
    printf("  | |  | | |___|  _ <| |___  \n");
    printf("  |_|  |_|_____|_| \\_\\_____|\n\n");

    // Print color test
    printf("Color test: ");
    for (int i = 30; i <= 37; ++i) {
        printf("\033[%dm██\033[0m", i);
    }
    printf("\n");

    // Print system info
    printf("Firmware: %s\n", FIRMWARE_VERSION);
    printf("OS: %s\n", OS_VERSION);
    printf("BIOS: %s\n", BIOS);
    printf("Total Memory: %d MB\n", TOTAL_MEMORY_MB);
}