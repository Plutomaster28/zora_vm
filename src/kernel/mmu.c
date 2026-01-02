#include "kernel/mmu.h"
#include "kernel/privilege.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>  // For VirtualAlloc

// Windows-compatible aligned allocation
static void* windows_aligned_alloc(size_t alignment, size_t size) {
    // Use VirtualAlloc for page-aligned allocation on Windows
    if (alignment == PAGE_SIZE) {
        return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    // Fallback to regular malloc for non-page alignment
    return malloc(size);
}

static void windows_aligned_free(void* ptr, size_t size) {
    // Check if this looks like a VirtualAlloc'd pointer (page-aligned)
    if (((uintptr_t)ptr & (PAGE_SIZE - 1)) == 0) {
        VirtualFree(ptr, 0, MEM_RELEASE);
    } else {
        free(ptr);
    }
}

// Global MMU state
static PageDirectory* g_kernel_page_directory = NULL;
static PageDirectory* g_current_page_directory = NULL;
static FrameAllocator g_frame_allocator;
static TLBStats g_tlb_stats;
static int g_paging_enabled = 0;
static int g_mmu_initialized = 0;

// Physical memory size (64MB as defined in original memory.c)
#define PHYSICAL_MEMORY_SIZE (64 * 1024 * 1024)
#define TOTAL_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)

// Frame bitmap
static uint32_t g_frame_bitmap[TOTAL_FRAMES / 32];

// Helper: Check if frame is allocated
static int is_frame_allocated(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    return (g_frame_bitmap[index] & (1 << bit)) != 0;
}

// Helper: Set frame as allocated
static void set_frame_allocated(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    g_frame_bitmap[index] |= (1 << bit);
}

// Helper: Set frame as free
static void set_frame_free(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    g_frame_bitmap[index] &= ~(1 << bit);
}

// Initialize MMU
int mmu_init(void) {
    if (g_mmu_initialized) {
        return 0;  // Already initialized
    }
    
    printf("[MMU] Initializing memory management unit...\n");
    
    // Initialize frame allocator
    memset(&g_frame_allocator, 0, sizeof(FrameAllocator));
    memset(g_frame_bitmap, 0, sizeof(g_frame_bitmap));
    
    g_frame_allocator.total_frames = TOTAL_FRAMES;
    g_frame_allocator.free_frames = TOTAL_FRAMES;
    g_frame_allocator.used_frames = 0;
    g_frame_allocator.first_free_frame = 0;
    
    // Initialize TLB stats
    memset(&g_tlb_stats, 0, sizeof(TLBStats));
    
    // Create kernel page directory
    g_kernel_page_directory = mmu_create_page_directory();
    if (!g_kernel_page_directory) {
        printf("[MMU] Failed to create kernel page directory\n");
        return -1;
    }
    
    // Identity map kernel space (only first 64MB for now, not full 1GB)
    // Mapping 1GB would require too much memory for page tables
    printf("[MMU] Identity mapping kernel space (0x00000000 - 0x03FFFFFF = 64MB)...\n");
    mmu_identity_map_range(g_kernel_page_directory, 0x00000000, PHYSICAL_MEMORY_SIZE,
                          PAGE_PRESENT | PAGE_WRITABLE | PAGE_KERNEL);
    
    // Set as current page directory
    g_current_page_directory = g_kernel_page_directory;
    
    g_mmu_initialized = 1;
    
    printf("[MMU] Initialized successfully\n");
    printf("[MMU] Total frames: %d (%d MB)\n", g_frame_allocator.total_frames,
           g_frame_allocator.total_frames * PAGE_SIZE / 1024 / 1024);
    
    return 0;
}

// Cleanup MMU
void mmu_cleanup(void) {
    if (!g_mmu_initialized) return;
    
    if (g_kernel_page_directory) {
        mmu_destroy_page_directory(g_kernel_page_directory);
        g_kernel_page_directory = NULL;
    }
    
    g_current_page_directory = NULL;
    g_mmu_initialized = 0;
    
    printf("[MMU] Cleaned up\n");
}

// Enable paging
void mmu_enable_paging(void) {
    if (!g_mmu_initialized || g_paging_enabled) return;
    
    // In real kernel, would load CR3 with page directory address and set CR0.PG bit
    // For simulation, just mark as enabled
    g_paging_enabled = 1;
    
    printf("[MMU] Paging enabled\n");
}

// Disable paging
void mmu_disable_paging(void) {
    if (!g_paging_enabled) return;
    
    // In real kernel, would clear CR0.PG bit
    g_paging_enabled = 0;
    
    printf("[MMU] Paging disabled\n");
}

// Create new page directory
PageDirectory* mmu_create_page_directory(void) {
    PageDirectory* dir = (PageDirectory*)windows_aligned_alloc(PAGE_SIZE, sizeof(PageDirectory));
    if (!dir) {
        return NULL;
    }
    
    memset(dir, 0, sizeof(PageDirectory));
    return dir;
}

// Destroy page directory
void mmu_destroy_page_directory(PageDirectory* dir) {
    if (!dir) return;
    
    // Free all page tables
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (dir->entries[i].present) {
            PageTable* table = (PageTable*)(uintptr_t)(dir->entries[i].table_addr << PAGE_SHIFT);
            windows_aligned_free(table, sizeof(PageTable));
        }
    }
    
    windows_aligned_free(dir, sizeof(PageDirectory));
}

// Switch page directory
void mmu_switch_page_directory(PageDirectory* dir) {
    if (!dir) return;
    
    g_current_page_directory = dir;
    
    // In real kernel, would load CR3 with directory physical address
    // This automatically flushes TLB
    g_tlb_stats.full_flushes++;
    
    printf("[MMU] Switched page directory\n");
}

// Get kernel page directory
PageDirectory* mmu_get_kernel_page_directory(void) {
    return g_kernel_page_directory;
}

// Get current page directory
PageDirectory* mmu_get_current_page_directory(void) {
    return g_current_page_directory;
}

// Allocate physical frame
uint32_t mmu_alloc_frame(void) {
    // Find first free frame
    for (uint32_t frame = g_frame_allocator.first_free_frame; 
         frame < g_frame_allocator.total_frames; frame++) {
        if (!is_frame_allocated(frame)) {
            set_frame_allocated(frame);
            g_frame_allocator.used_frames++;
            g_frame_allocator.free_frames--;
            g_frame_allocator.first_free_frame = frame + 1;
            return frame;
        }
    }
    
    printf("[MMU] Out of physical memory!\n");
    return 0xFFFFFFFF;
}

// Free physical frame
void mmu_free_frame(uint32_t frame) {
    if (frame >= g_frame_allocator.total_frames) return;
    
    if (is_frame_allocated(frame)) {
        set_frame_free(frame);
        g_frame_allocator.used_frames--;
        g_frame_allocator.free_frames++;
        
        if (frame < g_frame_allocator.first_free_frame) {
            g_frame_allocator.first_free_frame = frame;
        }
    }
}

// Check if frame is allocated
int mmu_is_frame_allocated(uint32_t frame) {
    if (frame >= g_frame_allocator.total_frames) return 0;
    return is_frame_allocated(frame);
}

// Get or create page table for directory entry
static PageTable* get_or_create_page_table(PageDirectory* dir, uint32_t pd_index) {
    if (!dir) return NULL;
    
    // If page table exists, return it
    if (dir->entries[pd_index].present) {
        return (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    }
    
    // Allocate new page table
    PageTable* table = (PageTable*)windows_aligned_alloc(PAGE_SIZE, sizeof(PageTable));
    if (!table) {
        return NULL;
    }
    
    memset(table, 0, sizeof(PageTable));
    
    // Setup directory entry
    dir->entries[pd_index].present = 1;
    dir->entries[pd_index].writable = 1;
    dir->entries[pd_index].user_mode = 1;
    dir->entries[pd_index].table_addr = (uint32_t)((uintptr_t)table >> PAGE_SHIFT);
    
    return table;
}

// Map virtual page to physical frame
int mmu_map_page(PageDirectory* dir, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    if (!dir) return -1;
    
    // Extract indices
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    
    // Get or create page table
    PageTable* table = get_or_create_page_table(dir, pd_index);
    if (!table) {
        return -1;
    }
    
    // Setup page table entry
    table->entries[pt_index].present = (flags & PAGE_PRESENT) ? 1 : 0;
    table->entries[pt_index].writable = (flags & PAGE_WRITABLE) ? 1 : 0;
    table->entries[pt_index].user_mode = (flags & PAGE_USER) ? 1 : 0;
    table->entries[pt_index].write_through = (flags & PAGE_WRITE_THROUGH) ? 1 : 0;
    table->entries[pt_index].cache_disable = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;
    table->entries[pt_index].accessed = 0;
    table->entries[pt_index].dirty = 0;
    table->entries[pt_index].global = (flags & PAGE_GLOBAL) ? 1 : 0;
    table->entries[pt_index].frame = physical_addr >> PAGE_SHIFT;
    
    // Invalidate TLB for this page
    mmu_flush_tlb_single(virtual_addr);
    
    return 0;
}

// Unmap virtual page
int mmu_unmap_page(PageDirectory* dir, uint32_t virtual_addr) {
    if (!dir) return -1;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    
    if (!dir->entries[pd_index].present) {
        return 0;  // Not mapped
    }
    
    PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    
    if (table->entries[pt_index].present) {
        // Free physical frame
        uint32_t frame = table->entries[pt_index].frame;
        mmu_free_frame(frame);
        
        // Clear entry
        memset(&table->entries[pt_index], 0, sizeof(PageTableEntry));
        
        // Invalidate TLB
        mmu_flush_tlb_single(virtual_addr);
    }
    
    return 0;
}

// Map range of pages
int mmu_map_range(PageDirectory* dir, uint32_t virtual_start, uint32_t physical_start, 
                  uint32_t size, uint32_t flags) {
    uint32_t virtual_addr = PAGE_ALIGN_DOWN(virtual_start);
    uint32_t physical_addr = PAGE_ALIGN_DOWN(physical_start);
    uint32_t end_addr = virtual_start + size;  // Don't align up to prevent overflow
    
    // Safety check
    if (size == 0 || size > 0x80000000) {  // Max 2GB
        printf("[MMU] ERROR: Invalid size for mmu_map_range: %u bytes\n", size);
        return -1;
    }
    
    int pages_mapped = 0;
    while (virtual_addr < end_addr && pages_mapped < 100000) {  // Safety limit
        if (mmu_map_page(dir, virtual_addr, physical_addr, flags) < 0) {
            return -1;
        }
        
        virtual_addr += PAGE_SIZE;
        physical_addr += PAGE_SIZE;
        pages_mapped++;
    }
    
    if (pages_mapped >= 100000) {
        printf("[MMU] WARNING: Mapped max limit of 100000 pages\n");
    }
    
    return 0;
}

// Unmap range of pages
int mmu_unmap_range(PageDirectory* dir, uint32_t virtual_start, uint32_t size) {
    uint32_t virtual_addr = PAGE_ALIGN_DOWN(virtual_start);
    uint32_t end_addr = PAGE_ALIGN_UP(virtual_start + size);
    
    while (virtual_addr < end_addr) {
        mmu_unmap_page(dir, virtual_addr);
        virtual_addr += PAGE_SIZE;
    }
    
    return 0;
}

// Translate virtual to physical address
uint32_t mmu_virtual_to_physical(PageDirectory* dir, uint32_t virtual_addr) {
    if (!dir) return 0xFFFFFFFF;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    uint32_t offset = PAGE_OFFSET(virtual_addr);
    
    if (!dir->entries[pd_index].present) {
        return 0xFFFFFFFF;  // Not mapped
    }
    
    PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    
    if (!table->entries[pt_index].present) {
        return 0xFFFFFFFF;  // Not mapped
    }
    
    uint32_t frame = table->entries[pt_index].frame;
    return (frame << PAGE_SHIFT) | offset;
}

// Check if virtual address is mapped
int mmu_is_mapped(PageDirectory* dir, uint32_t virtual_addr) {
    return (mmu_virtual_to_physical(dir, virtual_addr) != 0xFFFFFFFF);
}

// Identity map range (virtual = physical)
void mmu_identity_map_range(PageDirectory* dir, uint32_t physical_start, uint32_t size, uint32_t flags) {
    mmu_map_range(dir, physical_start, physical_start, size, flags);
}

// Page fault handler
void mmu_page_fault_handler(uint32_t fault_addr, uint32_t error_code) {
    printf("[MMU] Page fault at 0x%08X (error: 0x%02X)\n", fault_addr, error_code);
    
    // Decode error code
    int present = error_code & PF_PROTECTION;
    int write = error_code & PF_WRITE;
    int user = error_code & PF_USER;
    int reserved = error_code & PF_RESERVED;
    int instruction = error_code & PF_INSTRUCTION;
    
    printf("[MMU]   %s violation\n", present ? "Protection" : "Not present");
    printf("[MMU]   Access type: %s\n", write ? "Write" : "Read");
    printf("[MMU]   Mode: %s\n", user ? "User" : "Kernel");
    
    if (reserved) printf("[MMU]   Reserved bit violation\n");
    if (instruction) printf("[MMU]   Instruction fetch\n");
    
    // Check for copy-on-write
    if (present && write) {
        if (mmu_handle_cow_fault(fault_addr) == 0) {
            return;  // COW handled successfully
        }
    }
    
    // Unhandled page fault - would kill process in real kernel
    printf("[MMU] Unhandled page fault - halting\n");
}

// Handle copy-on-write fault
int mmu_handle_cow_fault(uint32_t virtual_addr) {
    // Check if page is marked as COW
    PageDirectory* dir = g_current_page_directory;
    if (!dir) return -1;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    
    if (!dir->entries[pd_index].present) return -1;
    
    PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    PageTableEntry* entry = &table->entries[pt_index];
    
    if (!entry->present) return -1;
    
    // Check if COW (using available bits)
    // For now, just return -1 (not implemented)
    return -1;
}

// Flush entire TLB
void mmu_flush_tlb(void) {
    // In real kernel, would reload CR3
    g_tlb_stats.full_flushes++;
    g_tlb_stats.entries_flushed += PAGE_DIRECTORY_ENTRIES * PAGE_TABLE_ENTRIES;
}

// Flush single TLB entry
void mmu_flush_tlb_single(uint32_t virtual_addr) {
    // In real kernel, would use INVLPG instruction
    g_tlb_stats.single_flushes++;
    g_tlb_stats.entries_flushed++;
}

// Invalidate page
void mmu_invalidate_page(uint32_t virtual_addr) {
    mmu_flush_tlb_single(virtual_addr);
}

// Set page flags
int mmu_set_page_flags(PageDirectory* dir, uint32_t virtual_addr, uint32_t flags) {
    if (!dir) return -1;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    
    if (!dir->entries[pd_index].present) return -1;
    
    PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    PageTableEntry* entry = &table->entries[pt_index];
    
    if (!entry->present) return -1;
    
    // Update flags
    entry->writable = (flags & PAGE_WRITABLE) ? 1 : 0;
    entry->user_mode = (flags & PAGE_USER) ? 1 : 0;
    
    mmu_flush_tlb_single(virtual_addr);
    
    return 0;
}

// Get page flags
uint32_t mmu_get_page_flags(PageDirectory* dir, uint32_t virtual_addr) {
    if (!dir) return 0;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    uint32_t pt_index = PT_INDEX(virtual_addr);
    
    if (!dir->entries[pd_index].present) return 0;
    
    PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
    PageTableEntry* entry = &table->entries[pt_index];
    
    if (!entry->present) return 0;
    
    uint32_t flags = 0;
    if (entry->present) flags |= PAGE_PRESENT;
    if (entry->writable) flags |= PAGE_WRITABLE;
    if (entry->user_mode) flags |= PAGE_USER;
    
    return flags;
}

// Protect memory range
int mmu_protect_range(PageDirectory* dir, uint32_t virtual_start, uint32_t size, uint32_t flags) {
    uint32_t virtual_addr = PAGE_ALIGN_DOWN(virtual_start);
    uint32_t end_addr = PAGE_ALIGN_UP(virtual_start + size);
    
    while (virtual_addr < end_addr) {
        mmu_set_page_flags(dir, virtual_addr, flags);
        virtual_addr += PAGE_SIZE;
    }
    
    return 0;
}

// Get statistics
void mmu_get_stats(uint32_t* total_pages, uint32_t* used_pages, uint32_t* free_pages) {
    if (total_pages) *total_pages = g_frame_allocator.total_frames;
    if (used_pages) *used_pages = g_frame_allocator.used_frames;
    if (free_pages) *free_pages = g_frame_allocator.free_frames;
}

// Get TLB stats
TLBStats* mmu_get_tlb_stats(void) {
    return &g_tlb_stats;
}

// Dump page tables
void mmu_dump_page_tables(PageDirectory* dir, uint32_t virtual_addr) {
    if (!dir) return;
    
    uint32_t pd_index = PD_INDEX(virtual_addr);
    
    printf("[MMU] Page directory entry %d:\n", pd_index);
    printf("  Present: %d\n", dir->entries[pd_index].present);
    printf("  Writable: %d\n", dir->entries[pd_index].writable);
    printf("  User: %d\n", dir->entries[pd_index].user_mode);
    
    if (dir->entries[pd_index].present) {
        PageTable* table = (PageTable*)(uintptr_t)(dir->entries[pd_index].table_addr << PAGE_SHIFT);
        uint32_t pt_index = PT_INDEX(virtual_addr);
        
        printf("  Page table entry %d:\n", pt_index);
        printf("    Present: %d\n", table->entries[pt_index].present);
        printf("    Writable: %d\n", table->entries[pt_index].writable);
        printf("    User: %d\n", table->entries[pt_index].user_mode);
        printf("    Frame: 0x%05X\n", table->entries[pt_index].frame);
    }
}
