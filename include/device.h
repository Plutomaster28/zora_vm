#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stddef.h>
#include <windows.h>

#define MAX_DEVICES 64
#define MAX_DEVICE_NAME 32
#define DEVICE_BUFFER_SIZE 4096

// Forward declarations
typedef struct Device Device;
typedef struct DeviceDriver DeviceDriver;

// Device classes
typedef enum {
    DEVICE_CLASS_STORAGE,
    DEVICE_CLASS_NETWORK,
    DEVICE_CLASS_INPUT,
    DEVICE_CLASS_OUTPUT,
    DEVICE_CLASS_DISPLAY,
    DEVICE_CLASS_AUDIO,
    DEVICE_CLASS_SERIAL,
    DEVICE_CLASS_PARALLEL,
    DEVICE_CLASS_USB,
    DEVICE_CLASS_PCI,
    DEVICE_CLASS_SYSTEM
} DeviceClass;

// Specific device types
typedef enum {
    // Storage devices
    DEVICE_HDD,
    DEVICE_SSD,
    DEVICE_FLOPPY,
    DEVICE_CDROM,
    DEVICE_USB_STORAGE,
    
    // Network devices
    DEVICE_ETHERNET,
    DEVICE_WIFI,
    DEVICE_BLUETOOTH,
    
    // Input devices
    DEVICE_KEYBOARD,
    DEVICE_MOUSE,
    DEVICE_JOYSTICK,
    DEVICE_TOUCHPAD,
    
    // Output devices
    DEVICE_TERMINAL,
    DEVICE_PRINTER,
    
    // Display devices
    DEVICE_VGA,
    DEVICE_HDMI,
    DEVICE_DP,
    
    // Audio devices
    DEVICE_SPEAKERS,
    DEVICE_MICROPHONE,
    DEVICE_SOUND_CARD,
    
    // System devices
    DEVICE_TIMER,
    DEVICE_RTC,
    DEVICE_PIC,
    DEVICE_DMA
} DeviceType;

// Device states
typedef enum {
    DEVICE_STATE_UNKNOWN,
    DEVICE_STATE_INITIALIZING,
    DEVICE_STATE_READY,
    DEVICE_STATE_BUSY,
    DEVICE_STATE_ERROR,
    DEVICE_STATE_OFFLINE,
    DEVICE_STATE_SUSPENDED
} DeviceState;

// Device capabilities
#define DEVICE_CAP_READ        0x0001
#define DEVICE_CAP_WRITE       0x0002
#define DEVICE_CAP_SEEK        0x0004
#define DEVICE_CAP_INTERRUPT   0x0008
#define DEVICE_CAP_DMA         0x0010
#define DEVICE_CAP_HOTPLUG     0x0020
#define DEVICE_CAP_POWER_MGMT  0x0040

// Device statistics
typedef struct {
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t operations_completed;
    uint64_t errors_encountered;
    uint64_t interrupts_generated;
    uint32_t uptime_seconds;
    uint32_t power_state_changes;
} DeviceStats;

// Device driver interface
struct DeviceDriver {
    char name[MAX_DEVICE_NAME];
    uint32_t version;
    DeviceClass class;
    
    // Driver function pointers
    int (*init)(Device* dev);
    int (*cleanup)(Device* dev);
    int (*read)(Device* dev, uint32_t address, void* buffer, size_t size);
    int (*write)(Device* dev, uint32_t address, const void* buffer, size_t size);
    int (*ioctl)(Device* dev, uint32_t command, void* data);
    int (*interrupt_handler)(Device* dev, uint32_t interrupt_data);
    int (*power_management)(Device* dev, uint32_t power_state);
};

// Main device structure
struct Device {
    // Basic device information
    uint32_t id;
    char name[MAX_DEVICE_NAME];
    DeviceClass class;
    DeviceType type;
    DeviceState state;
    
    // Hardware information
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t base_address;
    uint32_t memory_size;
    uint32_t irq_line;
    uint32_t capabilities;
    
    // Driver and data
    DeviceDriver* driver;
    void* device_data;
    void* buffer;
    
    // Performance and statistics
    DeviceStats stats;
    uint64_t last_access_time;
    
    // Synchronization
    CRITICAL_SECTION device_lock;
    
    // Power management
    uint32_t power_state;
    uint32_t wake_capabilities;
    
    // Next device in linked list
    struct Device* next;
};

// Device manager structure
typedef struct {
    Device* device_list;
    uint32_t device_count;
    uint32_t next_device_id;
    CRITICAL_SECTION manager_lock;
    
    // Device tree for hierarchical organization
    Device* root_devices[16];
    uint32_t root_device_count;
    
    // Statistics
    uint64_t total_interrupts;
    uint64_t total_io_operations;
    uint32_t devices_online;
    uint32_t devices_error;
} DeviceManager;

// Core device management functions
int device_manager_init(void);
void device_manager_cleanup(void);
DeviceManager* device_get_manager(void);

// Device lifecycle
Device* device_create(DeviceClass class, DeviceType type, const char* name);
int device_register(Device* device, DeviceDriver* driver);
int device_unregister(Device* device);
void device_destroy(Device* device);

// Device discovery and enumeration
int device_scan_pci_bus(void);
int device_scan_usb_bus(void);
void device_enumerate_all(void);
Device* device_find_by_id(uint32_t id);
Device* device_find_by_name(const char* name);
Device* device_find_by_type(DeviceType type);

// Device operations
int device_init(void);
void device_cleanup(void);
Device* device_get(int id);
int device_read(int id, uint32_t address, uint8_t* buffer, size_t size);
int device_write(int id, uint32_t address, const uint8_t* buffer, size_t size);
int device_ioctl(uint32_t device_id, uint32_t command, void* data);

// Device state management
int device_start(Device* device);
int device_stop(Device* device);
int device_reset(Device* device);
int device_suspend(Device* device);
int device_resume(Device* device);

// Interrupt and DMA support
int device_request_irq(Device* device, uint32_t irq_line);
void device_release_irq(Device* device);
void device_handle_interrupt(uint32_t irq_line);
int device_setup_dma(Device* device, void* buffer, size_t size);

// Power management
int device_set_power_state(Device* device, uint32_t power_state);
uint32_t device_get_power_state(Device* device);
void device_power_management_scan(void);

// Device statistics and monitoring
DeviceStats* device_get_statistics(Device* device);
void device_reset_statistics(Device* device);
void device_print_info(Device* device);
void device_print_all_devices(void);

// Hot-plug support
void device_hotplug_event(DeviceType type, int connected);
void device_scan_for_changes(void);

// Device driver management
int device_register_driver(DeviceDriver* driver);
int device_unregister_driver(DeviceDriver* driver);
DeviceDriver* device_find_driver(DeviceClass class, DeviceType type);

// Legacy device support functions (for backward compatibility)
void init_devices(void);
void process_io_operations(void);
void manage_device_state(void);

#endif // DEVICE_H