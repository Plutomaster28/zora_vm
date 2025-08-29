#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "device.h"

// Global device manager
static DeviceManager g_device_manager = {0};
static Device g_devices[MAX_DEVICES];
static DeviceDriver g_drivers[MAX_DEVICES];
static int g_device_count = 0;
static int g_driver_count = 0;

// Forward declarations for built-in drivers
static int terminal_driver_init(Device* dev);
static int disk_driver_init(Device* dev);
static int network_driver_init(Device* dev);

// Built-in device drivers
static DeviceDriver g_builtin_drivers[] = {
    {
        .name = "Terminal Driver",
        .version = 0x00010000,
        .class = DEVICE_CLASS_OUTPUT,
        .init = terminal_driver_init,
        .cleanup = NULL,
        .read = NULL,
        .write = NULL,
        .ioctl = NULL,
        .interrupt_handler = NULL,
        .power_management = NULL
    },
    {
        .name = "Virtual Disk Driver",
        .version = 0x00010000,
        .class = DEVICE_CLASS_STORAGE,
        .init = disk_driver_init,
        .cleanup = NULL,
        .read = NULL,
        .write = NULL,
        .ioctl = NULL,
        .interrupt_handler = NULL,
        .power_management = NULL
    },
    {
        .name = "Virtual Network Driver", 
        .version = 0x00010000,
        .class = DEVICE_CLASS_NETWORK,
        .init = network_driver_init,
        .cleanup = NULL,
        .read = NULL,
        .write = NULL,
        .ioctl = NULL,
        .interrupt_handler = NULL,
        .power_management = NULL
    }
};

// Built-in driver implementations
static int terminal_driver_init(Device* dev) {
    printf("[DEVDRV] Terminal driver initialized for device %s\n", dev->name);
    dev->state = DEVICE_STATE_READY;
    dev->capabilities = DEVICE_CAP_READ | DEVICE_CAP_WRITE;
    return 0;
}

static int disk_driver_init(Device* dev) {
    printf("[DEVDRV] Disk driver initialized for device %s\n", dev->name);
    dev->state = DEVICE_STATE_READY;
    dev->capabilities = DEVICE_CAP_READ | DEVICE_CAP_WRITE | DEVICE_CAP_SEEK;
    dev->memory_size = 1024 * 1024 * 100; // 100MB virtual disk
    return 0;
}

static int network_driver_init(Device* dev) {
    printf("[DEVDRV] Network driver initialized for device %s\n", dev->name);
    dev->state = DEVICE_STATE_READY;
    dev->capabilities = DEVICE_CAP_READ | DEVICE_CAP_WRITE | DEVICE_CAP_INTERRUPT;
    return 0;
}

int device_manager_init(void) {
    memset(&g_device_manager, 0, sizeof(DeviceManager));
    InitializeCriticalSection(&g_device_manager.manager_lock);
    
    g_device_manager.next_device_id = 1;
    
    printf("[DEVMGR] Device manager initialized\n");
    
    // Register built-in drivers
    for (int i = 0; i < sizeof(g_builtin_drivers) / sizeof(DeviceDriver); i++) {
        device_register_driver(&g_builtin_drivers[i]);
    }
    
    return 0;
}

void device_manager_cleanup(void) {
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    // Cleanup all devices
    Device* current = g_device_manager.device_list;
    while (current) {
        Device* next = current->next;
        device_destroy(current);
        current = next;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
    DeleteCriticalSection(&g_device_manager.manager_lock);
    
    printf("[DEVMGR] Device manager cleaned up\n");
}

DeviceManager* device_get_manager(void) {
    return &g_device_manager;
}

Device* device_create(DeviceClass class, DeviceType type, const char* name) {
    if (g_device_count >= MAX_DEVICES) {
        printf("[DEVMGR] ERROR: Maximum number of devices reached\n");
        return NULL;
    }
    
    Device* device = &g_devices[g_device_count++];
    memset(device, 0, sizeof(Device));
    
    device->id = g_device_manager.next_device_id++;
    device->class = class;
    device->type = type;
    device->state = DEVICE_STATE_INITIALIZING;
    
    strncpy(device->name, name, MAX_DEVICE_NAME - 1);
    device->name[MAX_DEVICE_NAME - 1] = '\0';
    
    // Initialize device buffer
    device->buffer = malloc(DEVICE_BUFFER_SIZE);
    if (!device->buffer) {
        printf("[DEVMGR] ERROR: Failed to allocate device buffer\n");
        g_device_count--;
        return NULL;
    }
    
    InitializeCriticalSection(&device->device_lock);
    device->last_access_time = GetTickCount64();
    
    printf("[DEVMGR] Created device: ID=%d, Name=%s, Class=%d, Type=%d\n",
           device->id, device->name, device->class, device->type);
    
    return device;
}

int device_register(Device* device, DeviceDriver* driver) {
    if (!device || !driver) {
        return -1;
    }
    
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    device->driver = driver;
    
    // Initialize the device with its driver
    if (driver->init && driver->init(device) != 0) {
        printf("[DEVMGR] ERROR: Failed to initialize device %s\n", device->name);
        LeaveCriticalSection(&g_device_manager.manager_lock);
        return -1;
    }
    
    // Add to device list
    device->next = g_device_manager.device_list;
    g_device_manager.device_list = device;
    g_device_manager.device_count++;
    
    if (device->state == DEVICE_STATE_READY) {
        g_device_manager.devices_online++;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
    
    printf("[DEVMGR] Registered device %s with driver %s\n", 
           device->name, driver->name);
    
    return 0;
}

void device_destroy(Device* device) {
    if (!device) return;
    
    if (device->driver && device->driver->cleanup) {
        device->driver->cleanup(device);
    }
    
    if (device->buffer) {
        free(device->buffer);
        device->buffer = NULL;
    }
    
    if (device->device_data) {
        free(device->device_data);
        device->device_data = NULL;
    }
    
    DeleteCriticalSection(&device->device_lock);
    
    printf("[DEVMGR] Destroyed device %s\n", device->name);
}

Device* device_find_by_id(uint32_t id) {
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    Device* current = g_device_manager.device_list;
    while (current) {
        if (current->id == id) {
            LeaveCriticalSection(&g_device_manager.manager_lock);
            return current;
        }
        current = current->next;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
    return NULL;
}

Device* device_find_by_type(DeviceType type) {
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    Device* current = g_device_manager.device_list;
    while (current) {
        if (current->type == type) {
            LeaveCriticalSection(&g_device_manager.manager_lock);
            return current;
        }
        current = current->next;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
    return NULL;
}

int device_register_driver(DeviceDriver* driver) {
    if (!driver || g_driver_count >= MAX_DEVICES) {
        return -1;
    }
    
    memcpy(&g_drivers[g_driver_count], driver, sizeof(DeviceDriver));
    g_driver_count++;
    
    printf("[DEVMGR] Registered driver: %s v%d.%d\n", 
           driver->name, 
           (driver->version >> 16) & 0xFFFF,
           driver->version & 0xFFFF);
    
    return 0;
}

DeviceDriver* device_find_driver(DeviceClass class, DeviceType type) {
    for (int i = 0; i < g_driver_count; i++) {
        if (g_drivers[i].class == class) {
            return &g_drivers[i];
        }
    }
    return NULL;
}

void device_print_all_devices(void) {
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                        DEVICE MANAGER                         ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Total Devices: %-3d  Online: %-3d  Error: %-3d             ║\n",
           g_device_manager.device_count,
           g_device_manager.devices_online,
           g_device_manager.devices_error);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    Device* current = g_device_manager.device_list;
    while (current) {
        printf("║ ID: %-3d │ %-20s │ State: %-12s ║\n",
               current->id,
               current->name,
               current->state == DEVICE_STATE_READY ? "READY" :
               current->state == DEVICE_STATE_BUSY ? "BUSY" :
               current->state == DEVICE_STATE_ERROR ? "ERROR" :
               current->state == DEVICE_STATE_OFFLINE ? "OFFLINE" : "UNKNOWN");
        current = current->next;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
    
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

// Legacy functions for backward compatibility
int device_init(void) {
    if (device_manager_init() != 0) {
        return -1;
    }
    
    // Create and register basic devices
    Device* terminal = device_create(DEVICE_CLASS_OUTPUT, DEVICE_TERMINAL, "Virtual Terminal");
    if (terminal) {
        DeviceDriver* driver = device_find_driver(DEVICE_CLASS_OUTPUT, DEVICE_TERMINAL);
        if (driver) {
            device_register(terminal, driver);
        }
    }
    
    Device* disk = device_create(DEVICE_CLASS_STORAGE, DEVICE_HDD, "Virtual Disk");
    if (disk) {
        DeviceDriver* driver = device_find_driver(DEVICE_CLASS_STORAGE, DEVICE_HDD);
        if (driver) {
            device_register(disk, driver);
        }
    }
    
    Device* network = device_create(DEVICE_CLASS_NETWORK, DEVICE_ETHERNET, "Virtual Network");
    if (network) {
        DeviceDriver* driver = device_find_driver(DEVICE_CLASS_NETWORK, DEVICE_ETHERNET);
        if (driver) {
            device_register(network, driver);
        }
    }
    
    printf("[DEVMGR] Device subsystem initialized with %d devices\n", g_device_manager.device_count);
    return 0;
}

void device_cleanup(void) {
    device_manager_cleanup();
}

Device* device_get(int id) {
    return device_find_by_id((uint32_t)id);
}

int device_read(int id, uint32_t address, uint8_t* buffer, size_t size) {
    Device* dev = device_find_by_id((uint32_t)id);
    if (!dev || dev->state != DEVICE_STATE_READY) {
        return -1;
    }
    
    EnterCriticalSection(&dev->device_lock);
    
    dev->stats.bytes_read += size;
    dev->stats.operations_completed++;
    dev->last_access_time = GetTickCount64();
    
    // Call driver read function if available
    int result = 0;
    if (dev->driver && dev->driver->read) {
        result = dev->driver->read(dev, address, buffer, size);
    } else {
        // Default read operation
        printf("[DEVICE] Reading %zu bytes from device %s at address 0x%x\n", 
               size, dev->name, address);
    }
    
    LeaveCriticalSection(&dev->device_lock);
    return result;
}

int device_write(int id, uint32_t address, const uint8_t* buffer, size_t size) {
    Device* dev = device_find_by_id((uint32_t)id);
    if (!dev || dev->state != DEVICE_STATE_READY) {
        return -1;
    }
    
    EnterCriticalSection(&dev->device_lock);
    
    dev->stats.bytes_written += size;
    dev->stats.operations_completed++;
    dev->last_access_time = GetTickCount64();
    
    // Call driver write function if available
    int result = 0;
    if (dev->driver && dev->driver->write) {
        result = dev->driver->write(dev, address, buffer, size);
    } else {
        // Default write operation
        printf("[DEVICE] Writing %zu bytes to device %s at address 0x%x\n", 
               size, dev->name, address);
    }
    
    LeaveCriticalSection(&dev->device_lock);
    return result;
}

// Legacy function implementations
void init_devices(void) {
    device_init();
}

void process_io_operations(void) {
    // Process pending I/O operations for all devices
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    Device* current = g_device_manager.device_list;
    while (current) {
        if (current->state == DEVICE_STATE_BUSY) {
            // Simulate I/O completion
            current->state = DEVICE_STATE_READY;
        }
        current = current->next;
    }
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
}

void manage_device_state(void) {
    // Monitor and manage device states
    EnterCriticalSection(&g_device_manager.manager_lock);
    
    uint32_t online_count = 0;
    uint32_t error_count = 0;
    
    Device* current = g_device_manager.device_list;
    while (current) {
        if (current->state == DEVICE_STATE_READY) {
            online_count++;
        } else if (current->state == DEVICE_STATE_ERROR) {
            error_count++;
        }
        current = current->next;
    }
    
    g_device_manager.devices_online = online_count;
    g_device_manager.devices_error = error_count;
    
    LeaveCriticalSection(&g_device_manager.manager_lock);
}