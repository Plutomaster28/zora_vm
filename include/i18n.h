#ifndef I18N_H
#define I18N_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Supported languages
typedef enum {
    ZORA_LANG_ENGLISH = 0,
    ZORA_LANG_JAPANESE = 1,
    ZORA_LANG_CHINESE_SIMPLIFIED = 2,
    ZORA_LANG_KOREAN = 3,
    ZORA_LANG_COUNT = 4
} language_t;

// String IDs for all translatable text
typedef enum {
    // Shell prompts and basic UI
    STR_HELP_HEADER,
    STR_HELP_DESCRIPTION,
    STR_HELP_AVAILABLE_COMMANDS,
    STR_HELP_EXAMPLES,
    STR_UNKNOWN_COMMAND,
    STR_TYPE_HELP,
    STR_EXITING_VM,
    STR_CHANGED_DIRECTORY,
    STR_DIRECTORY_NOT_FOUND,
    STR_CONTENTS_OF,
    STR_EMPTY_DIRECTORY,
    STR_FILE_NOT_FOUND,
    STR_ERROR_OPENING_FILE,
    STR_ERROR_READING_FILE,
    STR_PERMISSION_DENIED,
    STR_INVALID_ARGUMENTS,
    
    // Kernel and boot messages
    STR_KERNEL_BOOT_SEQUENCE,
    STR_KERNEL_JAVA_WARNING,
    STR_KERNEL_PANIC_JAVA,
    STR_DEVICE_MANAGER_INIT,
    STR_DEVICE_REGISTERED,
    STR_VFS_MOUNTED,
    STR_SYSTEM_READY,
    
    // Language system
    STR_CURRENT_LANGUAGE,
    STR_LANGUAGE_CHANGED,
    STR_INVALID_LANGUAGE,
    STR_AVAILABLE_LANGUAGES,
    STR_LANGUAGE_HELP,
    
    // Error messages
    STR_INPUT_CORRUPTION,
    STR_CLEARING_BUFFERS,
    STR_MEMORY_ERROR,
    STR_SYSTEM_ERROR,
    
    // Command descriptions
    STR_CMD_HELP_DESC,
    STR_CMD_LS_DESC,
    STR_CMD_CD_DESC,
    STR_CMD_CAT_DESC,
    STR_CMD_EXIT_DESC,
    STR_CMD_CLEAR_DESC,
    STR_CMD_LANG_DESC,
    
    STR_COUNT // Must be last
} string_id_t;

// Language information structure
typedef struct {
    const char* name_english;
    const char* name_native;
    const char* iso_code;
    const char* encoding;
} language_info_t;

// Function declarations
void i18n_init(void);
void i18n_set_language(language_t lang);
language_t i18n_get_language(void);
const char* i18n_get_string(string_id_t id);
const char* i18n_get_language_name(language_t lang);
const char* i18n_get_language_native_name(language_t lang);
language_t i18n_parse_language(const char* lang_str);
void i18n_list_languages(void);

// Convenience macro for getting localized strings
#define _(id) i18n_get_string(id)

// Language persistence
void i18n_save_language_preference(void);
void i18n_load_language_preference(void);

#endif // I18N_H