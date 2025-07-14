#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_DEVICES 16

typedef enum {
    DEVICE_TERMINAL,
    DEVICE_DISK,
    DEVICE_NETWORK
} DeviceType;

typedef enum {
    DEVICE_STATUS_READY,
    DEVICE_STATUS_BUSY,
    DEVICE_STATUS_ERROR
} DeviceStatus;

typedef struct {
    int id;
    DeviceType type;
    DeviceStatus status;
    void* data;
} Device;

// Function prototypes for device management
void init_devices();
void process_io_operations();
void manage_device_state();

// Device functions
int device_init(void);
void device_cleanup(void);
Device* device_get(int id);
int device_read(int id, uint32_t address, uint8_t* buffer, size_t size);
int device_write(int id, uint32_t address, const uint8_t* buffer, size_t size);

#endif // DEVICE_H