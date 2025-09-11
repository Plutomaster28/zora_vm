#ifndef JAVA_DETECTOR_H
#define JAVA_DETECTOR_H

// Java Detection and Emergency Response System
typedef struct {
    int java_detected;
    int threat_level;
    char detected_files[10][256];
    int file_count;
    char violation_type[64];
} JavaThreatAssessment;

// Java detection functions
int java_detector_init(void);
int java_scan_directory(const char* path);
int java_check_file(const char* filename);
int java_check_file_content(const char* filepath);
void java_trigger_kernel_panic(JavaThreatAssessment* threat);
void java_display_bsod(JavaThreatAssessment* threat);
int java_is_extension_dangerous(const char* filename);
int java_scan_for_keywords(const char* content);

// Emergency response
void java_emergency_shutdown(void);
void java_quarantine_system(void);

#endif // JAVA_DETECTOR_H
