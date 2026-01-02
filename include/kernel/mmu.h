#ifndef KERNEL_MMU_H
#define KERNEL_MMU_H

#include <stdint.h>

// Page size and masks
#define PAGE_SIZE           4096        // 4KB pages
#define PAGE_SHIFT          12          // 2^12 = 4096
#define PAGE_MASK           0xFFFFF000  // Mask for page-aligned addresses
#define PAGE_OFFSET_MASK    0x00000FFF  // Mask for offset within page

// Page directory and table sizes
#define PAGE_DIRECTORY_ENTRIES  1024    // 1024 entries in page directory
#define PAGE_TABLE_ENTRIES      1024    // 1024 entries per page table
#define PAGES_PER_TABLE         1024
#define TABLES_PER_DIRECTORY    1024

// Page flags
#define PAGE_PRESENT        0x001   // Page is present in memory
#define PAGE_WRITABLE       0x002   // Page is writable
#define PAGE_USER           0x004   // Page is accessible from user mode
#define PAGE_WRITE_THROUGH  0x008   // Write-through caching
#define PAGE_CACHE_DISABLE  0x010   // Cache disabled
#define PAGE_ACCESSED       0x020   // Page has been accessed
#define PAGE_DIRTY          0x040   // Page has been written to
#define PAGE_SIZE_FLAG      0x080   // Page size (0=4KB, 1=4MB)
#define PAGE_GLOBAL         0x100   // Global page (not flushed on CR3 reload)
#define PAGE_KERNEL         0x200   // Kernel-only page (custom flag)
#define PAGE_COW            0x400   // Copy-on-write (custom flag)
#define PAGE_SWAPPED        0x800   // Page is swapped out (custom flag)

// Virtual memory regions
#define KERNEL_SPACE_START  0x00000000  // Kernel space: 0x00000000 - 0x3FFFFFFF (1GB)
#define KERNEL_SPACE_END    0x3FFFFFFF
#define USER_SPACE_START    0x40000000  // User space: 0x40000000 - 0xFFFFFFFF (3GB)
#define USER_SPACE_END      0xFFFFFFFF

#define KERNEL_HEAP_START   0x10000000  // Kernel heap starts at 256MB
#define KERNEL_HEAP_SIZE    0x10000000  // 256MB kernel heap
#define KERNEL_STACK_START  0x20000000  // Kernel stacks at 512MB
#define KERNEL_STACK_SIZE   0x00400000  // 4MB kernel stack region

// Page table entry
typedef struct {
    uint32_t present        : 1;    // Present in memory
    uint32_t writable       : 1;    // Writable
    uint32_t user_mode      : 1;    // User mode accessible
    uint32_t write_through  : 1;    // Write-through caching
    uint32_t cache_disable  : 1;    // Cache disabled
    uint32_t accessed       : 1;    // Accessed
    uint32_t dirty          : 1;    // Dirty (written to)
    uint32_t pat            : 1;    // Page Attribute Table
    uint32_t global         : 1;    // Global page
    uint32_t available      : 3;    // Available for OS use
    uint32_t frame          : 20;   // Physical frame address (bits 12-31)
} __attribute__((packed)) PageTableEntry;

// Page table
typedef struct {
    PageTableEntry entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) PageTable;

// Page directory entry
typedef struct {
    uint32_t present        : 1;
    uint32_t writable       : 1;
    uint32_t user_mode      : 1;
    uint32_t write_through  : 1;
    uint32_t cache_disable  : 1;
    uint32_t accessed       : 1;
    uint32_t available1     : 1;
    uint32_t page_size      : 1;    // 0=4KB, 1=4MB
    uint32_t available2     : 4;
    uint32_t table_addr     : 20;   // Page table physical address
} __attribute__((packed)) PageDirectoryEntry;

// Page directory
typedef struct {
    PageDirectoryEntry entries[PAGE_DIRECTORY_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) PageDirectory;

// Page fault error codes
#define PF_PROTECTION   0x01    // Protection violation (vs. non-present)
#define PF_WRITE        0x02    // Write access (vs. read)
#define PF_USER         0x04    // User mode (vs. kernel)
#define PF_RESERVED     0x08    // Reserved bit violation
#define PF_INSTRUCTION  0x10    // Instruction fetch

// Physical frame allocator
typedef struct {
    uint32_t* bitmap;           // Bitmap of free frames
    uint32_t total_frames;      // Total number of frames
    uint32_t used_frames;       // Number of used frames
    uint32_t free_frames;       // Number of free frames
    uint32_t first_free_frame;  // Hint for next allocation
} FrameAllocator;

// TLB management
typedef struct {
    uint32_t entries_flushed;
    uint32_t full_flushes;
    uint32_t single_flushes;
} TLBStats;

// MMU initialization and cleanup
int mmu_init(void);
void mmu_cleanup(void);
void mmu_enable_paging(void);
void mmu_disable_paging(void);

// Page directory management
PageDirectory* mmu_create_page_directory(void);
void mmu_destroy_page_directory(PageDirectory* dir);
void mmu_switch_page_directory(PageDirectory* dir);
PageDirectory* mmu_get_kernel_page_directory(void);
PageDirectory* mmu_get_current_page_directory(void);

// Page mapping
int mmu_map_page(PageDirectory* dir, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
int mmu_unmap_page(PageDirectory* dir, uint32_t virtual_addr);
int mmu_map_range(PageDirectory* dir, uint32_t virtual_start, uint32_t physical_start, uint32_t size, uint32_t flags);
int mmu_unmap_range(PageDirectory* dir, uint32_t virtual_start, uint32_t size);

// Address translation
uint32_t mmu_virtual_to_physical(PageDirectory* dir, uint32_t virtual_addr);
int mmu_is_mapped(PageDirectory* dir, uint32_t virtual_addr);

// Page fault handling
void mmu_page_fault_handler(uint32_t fault_addr, uint32_t error_code);
int mmu_handle_cow_fault(uint32_t virtual_addr);

// TLB management
void mmu_flush_tlb(void);
void mmu_flush_tlb_single(uint32_t virtual_addr);
void mmu_invalidate_page(uint32_t virtual_addr);

// Frame allocation
uint32_t mmu_alloc_frame(void);
void mmu_free_frame(uint32_t frame);
int mmu_is_frame_allocated(uint32_t frame);

// Memory protection
int mmu_set_page_flags(PageDirectory* dir, uint32_t virtual_addr, uint32_t flags);
uint32_t mmu_get_page_flags(PageDirectory* dir, uint32_t virtual_addr);
int mmu_protect_range(PageDirectory* dir, uint32_t virtual_start, uint32_t size, uint32_t flags);

// Identity mapping (for kernel)
void mmu_identity_map_range(PageDirectory* dir, uint32_t physical_start, uint32_t size, uint32_t flags);

// Statistics
void mmu_get_stats(uint32_t* total_pages, uint32_t* used_pages, uint32_t* free_pages);
TLBStats* mmu_get_tlb_stats(void);
void mmu_dump_page_tables(PageDirectory* dir, uint32_t virtual_addr);

// Helper macros
#define PAGE_ALIGN_DOWN(addr)   ((addr) & PAGE_MASK)
#define PAGE_ALIGN_UP(addr)     (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define IS_PAGE_ALIGNED(addr)   (((addr) & PAGE_OFFSET_MASK) == 0)
#define ADDR_TO_PAGE(addr)      ((addr) >> PAGE_SHIFT)
#define PAGE_TO_ADDR(page)      ((page) << PAGE_SHIFT)

#define PD_INDEX(addr)          ((addr) >> 22)
#define PT_INDEX(addr)          (((addr) >> 12) & 0x3FF)
#define PAGE_OFFSET(addr)       ((addr) & 0xFFF)

#endif // KERNEL_MMU_H
