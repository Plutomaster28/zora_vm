#include <stdio.h>
#include <stdlib.h>
#include "device.h"

static Device devices[MAX_DEVICES];
static int device_count = 0;

int device_init(void) {
    device_count = 0;
    
    // Initialize basic devices
    devices[0].id = 0;
    devices[0].type = DEVICE_TERMINAL;
    devices[0].status = DEVICE_STATUS_READY;
    device_count++;
    
    devices[1].id = 1;
    devices[1].type = DEVICE_DISK;
    devices[1].status = DEVICE_STATUS_READY;
    device_count++;
    
    printf("Devices initialized: %d devices\n", device_count);
    return 0;
}

void device_cleanup(void) {
    device_count = 0;
    printf("Devices cleaned up\n");
}

Device* device_get(int id) {
    if (id < 0 || id >= device_count) {
        return NULL;
    }
    return &devices[id];
}

int device_read(int id, uint32_t address, uint8_t* buffer, size_t size) {
    Device* dev = device_get(id);
    if (!dev || dev->status != DEVICE_STATUS_READY) {
        return -1;
    }
    
    // Device-specific read logic
    return 0;
}

int device_write(int id, uint32_t address, const uint8_t* buffer, size_t size) {
    Device* dev = device_get(id);
    if (!dev || dev->status != DEVICE_STATUS_READY) {
        return -1;
    }
    
    // Device-specific write logic
    return 0;
}