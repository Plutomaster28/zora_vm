#include "../../include/kernel/java_detector.h"
#include "../../include/vfs/vfs.h"
#include "../../include/terminal/terminal_style.h"
#include "../../include/kernel.h"  // For kernel_panic()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

static JavaThreatAssessment current_threat;
static int java_detector_active = 0;

// Dangerous Java file extensions and keywords
static const char* java_extensions[] = {
    ".java", ".class", ".jar", ".war", ".ear", ".jsp", ".jspx", 
    ".jnlp", ".jad", ".properties", ".gradle", ".pom", NULL
};

static const char* java_keywords[] = {
    "public class", "import java", "package ", "extends ", "implements ",
    "public static void main", "System.out.println", "BufferedReader",
    "FileInputStream", "HttpServlet", "Spring", "Hibernate", "Maven",
    "Gradle", "JVM", "javax.", "org.apache", "com.sun", NULL
};

static const char* java_enterprise_horrors[] = {
    "AbstractSingletonProxyFactoryBean", "InternalFrameworkConfigurationException",
    "SimpleJdbcCallOperationNotFoundException", "TransactionProxyFactoryBean",
    "BeanCreationNotAllowedException", "NestedServletException", NULL
};

int java_detector_init(void) {
    memset(&current_threat, 0, sizeof(JavaThreatAssessment));
    java_detector_active = 1;
    
    printf("\n Java Detection System Initialized\n");
    printf("  WARNING: Java presence will trigger immediate kernel panic!\n");
    printf("  System protected against enterprise architecture patterns\n\n");
    
    return 1;
}

int java_scan_directory(const char* path) {
    if (!java_detector_active) return 0;
    
    DIR *dir;
    struct dirent *entry;
    char full_path[512];
    
    dir = opendir(path);
    if (!dir) return 0;
    
    printf(" Scanning directory: %s\n", path);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // Check if it's a directory
        DIR *test_dir = opendir(full_path);
        if (test_dir) {
            closedir(test_dir);
            // Recursively scan subdirectories
            if (java_scan_directory(full_path)) {
                closedir(dir);
                return 1; // Java detected in subdirectory
            }
        } else {
            // It's a file, check it
            if (java_check_file(entry->d_name)) {
                strncpy(current_threat.detected_files[current_threat.file_count], 
                       full_path, 255);
                current_threat.file_count++;
                current_threat.java_detected = 1;
                
                if (current_threat.file_count >= 10) {
                    strcpy(current_threat.violation_type, "MASSIVE_JAVA_INFESTATION");
                    current_threat.threat_level = 10;
                    closedir(dir);
                    java_trigger_kernel_panic(&current_threat);
                    return 1;
                }
            }
            
            // Also check file content for stealth Java
            java_check_file_content(full_path);
        }
    }
    
    closedir(dir);
    
    if (current_threat.java_detected) {
        if (current_threat.file_count > 5) {
            current_threat.threat_level = 8;
            strcpy(current_threat.violation_type, "ENTERPRISE_JAVA_DETECTED");
        } else if (current_threat.file_count > 2) {
            current_threat.threat_level = 6;
            strcpy(current_threat.violation_type, "JAVA_FRAMEWORK_DETECTED");
        } else {
            current_threat.threat_level = 4;
            strcpy(current_threat.violation_type, "BASIC_JAVA_VIOLATION");
        }
        java_trigger_kernel_panic(&current_threat);
        return 1;
    }
    
    return 0;
}

int java_check_file(const char* filename) {
    // Check file extension
    for (int i = 0; java_extensions[i] != NULL; i++) {
        if (strstr(filename, java_extensions[i]) != NULL) {
            printf(" JAVA FILE DETECTED: %s\n", filename);
            return 1;
        }
    }
    
    // Check for suspicious Java-related names
    char lower_name[256];
    strncpy(lower_name, filename, 255);
    for (int i = 0; lower_name[i]; i++) {
        lower_name[i] = tolower(lower_name[i]);
    }
    
    if (strstr(lower_name, "java") || strstr(lower_name, "spring") || 
        strstr(lower_name, "hibernate") || strstr(lower_name, "maven") ||
        strstr(lower_name, "gradle") || strstr(lower_name, "tomcat")) {
        printf(" SUSPICIOUS JAVA-RELATED FILE: %s\n", filename);
        return 1;
    }
    
    return 0;
}

int java_check_file_content(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) return 0;
    
    char buffer[1024];
    int java_found = 0;
    
    // Read first few lines to check for Java content
    for (int line = 0; line < 20 && fgets(buffer, sizeof(buffer), file); line++) {
        // Convert to lowercase for case-insensitive matching
        char lower_buffer[1024];
        strncpy(lower_buffer, buffer, 1023);
        for (int i = 0; lower_buffer[i]; i++) {
            lower_buffer[i] = tolower(lower_buffer[i]);
        }
        
        // Check for Java keywords
        for (int i = 0; java_keywords[i] != NULL; i++) {
            if (strstr(lower_buffer, java_keywords[i]) != NULL) {
                printf(" STEALTH JAVA CODE DETECTED in %s: %s\n", filepath, java_keywords[i]);
                java_found = 1;
                break;
            }
        }
        
        // Check for enterprise horror patterns
        for (int i = 0; java_enterprise_horrors[i] != NULL; i++) {
            if (strstr(buffer, java_enterprise_horrors[i]) != NULL) {
                printf(" ENTERPRISE JAVA HORROR DETECTED: %s\n", java_enterprise_horrors[i]);
                strcpy(current_threat.violation_type, "ENTERPRISE_ARCHITECTURE_NIGHTMARE");
                current_threat.threat_level = 11; // Maximum threat
                java_found = 1;
                break;
            }
        }
        
        if (java_found) break;
    }
    
    fclose(file);
    
    if (java_found) {
        strncpy(current_threat.detected_files[current_threat.file_count], 
               filepath, 255);
        current_threat.file_count++;
        current_threat.java_detected = 1;
    }
    
    return java_found;
}

void java_trigger_kernel_panic(JavaThreatAssessment* threat) {
    printf("\n🚨 CRITICAL SYSTEM ERROR 🚨\n");
    printf("JAVA DETECTED - INITIATING EMERGENCY PROTOCOLS\n\n");
    
    // Brief delay for dramatic effect
    for (volatile int i = 0; i < 50000000; i++);
    
    // Display our custom Java BSOD first
    java_display_bsod(threat);
    
    // Now trigger the ACTUAL kernel panic with Java-specific error codes
    uint32_t java_error_code;
    char panic_message[512];
    
    if (threat->threat_level >= 11) {
        java_error_code = 0xE47E4491; // "ENTERPF1SE" approximation in hex
        snprintf(panic_message, sizeof(panic_message), 
                "ENTERPRISE JAVA NIGHTMARE: %s detected! AbstractSingletonProxyFactoryBean contamination!", 
                threat->violation_type);
    } else if (threat->threat_level >= 10) {
        java_error_code = 0xDEADBEEF; // Massive Java infestation  
        snprintf(panic_message, sizeof(panic_message), 
                "MASSIVE JAVA INFESTATION: %d files detected! System integrity compromised!", 
                threat->file_count);
    } else if (threat->threat_level >= 8) {
        java_error_code = 0xCAFFEBAD; // Coffee is bad (Java reference)
        snprintf(panic_message, sizeof(panic_message), 
                "ENTERPRISE JAVA FRAMEWORK DETECTED: %s - Spring/Hibernate contamination!", 
                threat->violation_type);
    } else if (threat->threat_level >= 6) {
        java_error_code = 0xC0FFEE; // Coffee/Java reference
        snprintf(panic_message, sizeof(panic_message), 
                "JAVA FRAMEWORK CONTAMINATION: %d files found! System refusing to continue!", 
                threat->file_count);
    } else {
        java_error_code = 0xBADC0DE; // Bad code (Java is bad code)
        snprintf(panic_message, sizeof(panic_message), 
                "BASIC JAVA VIOLATION: %s detected! Even basic Java is unacceptable!", 
                threat->violation_type);
    }
    
    printf("\n⚠️  TRIGGERING KERNEL PANIC ⚠️\n");
    printf("Passing control to Zora Kernel panic handler...\n\n");
    
    // Trigger the ACTUAL kernel panic - this will halt the system properly
    kernel_panic(java_error_code, panic_message);
    
    // This line should never be reached due to kernel_panic halting the system
    exit(java_error_code);
}

void java_display_bsod(JavaThreatAssessment* threat) {
    // Clear screen and display blue screen of death
    printf("\033[2J\033[H"); // Clear screen
    printf("\033[44m\033[37m"); // Blue background, white text
    
    // Top border
    for (int i = 0; i < 80; i++) printf("█");
    printf("\n");
    
    printf("█                           KERNEL PANIC - JAVA DETECTED                        █\n");
    printf("█                                                                               █\n");
    printf("█  A fatal exception has occurred due to the presence of Java code.            █\n");
    printf("█  The system has been halted to prevent enterprise architecture contamination.█\n");
    printf("█                                                                               █\n");
    printf("█  Violation Type: %-58s █\n", threat->violation_type);
    printf("█  Threat Level:   %-58d █\n", threat->threat_level);
    printf("█  Files Found:    %-58d █\n", threat->file_count);
    printf("█                                                                               █\n");
    printf("█  Detected Files:                                                              █\n");
    
    for (int i = 0; i < threat->file_count && i < 8; i++) {
        printf("█    %-74s █\n", threat->detected_files[i]);
    }
    
    if (threat->file_count > 8) {
        printf("█    ... and %d more files                                                    █\n", 
               threat->file_count - 8);
    }
    
    printf("█                                                                               █\n");
    printf("█  Error Code: 0x%08X (JAVA_CONTAMINATION_DETECTED)                          █\n", 
           0xDEADBEEF);
    printf("█                                                                               █\n");
    printf("█  Recommended Actions:                                                         █\n");
    printf("█  1. Remove all Java files immediately                                        █\n");
    printf("█  2. Purify system with C code                                                █\n");
    printf("█  3. Consider switching to Assembly for ultimate performance                  █\n");
    printf("█  4. Burn any Java Enterprise Edition books you may own                       █\n");
    printf("█                                                                               █\n");
    printf("█  If you continue to see this message, your system may be infected with       █\n");
    printf("█  AbstractSingletonProxyFactoryBean patterns. Please contact a C programmer. █\n");
    printf("█                                                                               █\n");
    
    // Bottom border
    for (int i = 0; i < 80; i++) printf("█");
    printf("\n");
    
    printf("\033[0m"); // Reset colors
    
    printf("\n SYSTEM HALTED \n");
    printf("Java contamination detected. ZoraVM refuses to continue.\n");
    printf("Please remove all Java files and restart the system.\n\n");
    
    // More dramatic messages based on threat level
    if (threat->threat_level >= 10) {
        printf(" MAXIMUM THREAT LEVEL DETECTED \n");
        printf("Enterprise Java patterns found. System entering emergency quarantine.\n");
        printf("Memory being wiped to prevent AbstractFactory contamination.\n");
    } else if (threat->threat_level >= 8) {
        printf(" HIGH THREAT: Java framework detected!\n");
        printf("Spring/Hibernate patterns may cause permanent brain damage.\n");
    } else if (threat->threat_level >= 6) {
        printf(" MODERATE THREAT: Multiple Java files found.\n");
        printf("Verbose stack traces may overwhelm terminal buffers.\n");
    } else {
        printf(" LOW THREAT: Basic Java detected.\n");
        printf("Still unacceptable. C is the only way.\n");
    }
}

void java_emergency_shutdown(void) {
    printf("\n INITIATING EMERGENCY SHUTDOWN SEQUENCE \n");
    printf("Flushing all buffers...\n");
    printf("Clearing Java bytecode from memory...\n");
    printf("Disabling JVM loader...\n");
    printf("Purging AbstractFactory instances...\n");
    printf("Reverting to safe C-only mode...\n\n");
    
    printf("System will now exit to protect against Java contamination.\n");
    printf("Please remove all Java files before restarting ZoraVM.\n\n");
    
    printf("Remember: Friends don't let friends use Java. \n\n");
    
    // Actually exit the program
    exit(0xDEADBEEF);
}

void java_quarantine_system(void) {
    printf(" Quarantine protocols activated.\n");
    printf("All Java-related processes terminated.\n");
    printf("System entering C-only safe mode.\n");
}
