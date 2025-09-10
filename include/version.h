#ifndef VERSION_H
#define VERSION_H

#include <stdio.h>
#include <string.h>

// =============================================================================
// ZORA VM AUTOMATIC VERSIONING SYSTEM
// =============================================================================
// This system automatically generates rolling version numbers based on build dates
// Major version increments yearly, minor monthly, patch daily

// Base version epoch (January 1, 2024)
#define VERSION_EPOCH_YEAR 2024
#define VERSION_EPOCH_MONTH 1
#define VERSION_EPOCH_DAY 1

// Extract build date components from __DATE__ macro
static const char* month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static inline int get_build_month() {
    char month_str[4];
    sscanf(__DATE__, "%3s", month_str);
    for (int i = 0; i < 12; i++) {
        if (strcmp(month_str, month_names[i]) == 0) {
            return i + 1;
        }
    }
    return 1; // Default to January if parsing fails
}

static inline int get_build_day() {
    int day;
    sscanf(__DATE__ + 4, "%d", &day);
    return day;
}

static inline int get_build_year() {
    int year;
    sscanf(__DATE__ + 7, "%d", &year);
    return year;
}

// Calculate days since epoch
static inline int days_since_epoch() {
    int build_year = get_build_year();
    int build_month = get_build_month();
    int build_day = get_build_day();
    
    // Simple day calculation (approximation for version numbering)
    int days = 0;
    
    // Add days for complete years
    for (int year = VERSION_EPOCH_YEAR; year < build_year; year++) {
        days += 365;
        // Add leap day for leap years
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days += 1;
        }
    }
    
    // Add days for complete months in current year
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int month = 1; month < build_month; month++) {
        days += days_in_month[month - 1];
        // Add leap day for February in leap years
        if (month == 2 && ((build_year % 4 == 0 && build_year % 100 != 0) || (build_year % 400 == 0))) {
            days += 1;
        }
    }
    
    // Add remaining days
    days += build_day - VERSION_EPOCH_DAY;
    
    return days;
}

// Generate automatic version numbers
static inline void get_zora_version(int* major, int* minor, int* patch, int* build) {
    int build_year = get_build_year();
    int build_month = get_build_month();
    int build_day = get_build_day();
    int total_days = days_since_epoch();
    
    // Major version: years since epoch + 2 (so we start at v3.x.x for 2024+)
    *major = 2 + (build_year - VERSION_EPOCH_YEAR);
    
    // Minor version: month of year (1-12)
    *minor = build_month;
    
    // Patch version: day of month (1-31)
    *patch = build_day;
    
    // Build number: total days since epoch (for uniqueness)
    *build = total_days;
}

// Get version string
static inline void get_zora_version_string(char* version_str, size_t size) {
    int major, minor, patch, build;
    get_zora_version(&major, &minor, &patch, &build);
    snprintf(version_str, size, "%d.%d.%d.%d", major, minor, patch, build);
}

// Get short version string (without build number)
static inline void get_zora_version_short(char* version_str, size_t size) {
    int major, minor, patch, build;
    get_zora_version(&major, &minor, &patch, &build);
    snprintf(version_str, size, "%d.%d.%d", major, minor, patch);
}

// Get version codename based on major.minor
static inline const char* get_version_codename() {
    int major, minor, patch, build;
    get_zora_version(&major, &minor, &patch, &build);
    
    // Generate codenames based on version
    static const char* codenames[] = {
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta",
        "Eta", "Theta", "Iota", "Kappa", "Lambda", "Mu"
    };
    
    int codename_index = ((major - 2) * 12 + (minor - 1)) % 12;
    return codenames[codename_index];
}

#endif // VERSION_H
