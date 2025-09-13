#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Current language setting
static language_t current_language = ZORA_LANG_ENGLISH;

// Language information table
static const language_info_t language_info[ZORA_LANG_COUNT] = {
    {"English", "English", "en", "UTF-8"},
    {"Japanese", "日本語", "ja", "UTF-8"},
    {"Chinese (Simplified)", "简体中文", "zh-CN", "UTF-8"},
    {"Korean", "한국어", "ko", "UTF-8"}
};

// Translation tables for all supported languages
static const char* translations[ZORA_LANG_COUNT][STR_COUNT] = {
    // English translations
    {
        "ZoraVM Help System",                                    // STR_HELP_HEADER
        "Advanced Virtual Machine Operating System",            // STR_HELP_DESCRIPTION
        "Available commands:",                                   // STR_HELP_AVAILABLE_COMMANDS
        "Examples:",                                            // STR_HELP_EXAMPLES
        "Unknown command",                                      // STR_UNKNOWN_COMMAND
        "Type help to see available commands.",                 // STR_TYPE_HELP
        "Exiting VM...",                                        // STR_EXITING_VM
        "Changed directory to:",                                // STR_CHANGED_DIRECTORY
        "Directory not found:",                                 // STR_DIRECTORY_NOT_FOUND
        "Contents of",                                          // STR_CONTENTS_OF
        "(empty directory)",                                    // STR_EMPTY_DIRECTORY
        "File not found:",                                      // STR_FILE_NOT_FOUND
        "Error opening file:",                                  // STR_ERROR_OPENING_FILE
        "Error reading file:",                                  // STR_ERROR_READING_FILE
        "Permission denied:",                                   // STR_PERMISSION_DENIED
        "Invalid arguments",                                    // STR_INVALID_ARGUMENTS
        
        "Boot sequence initiating...",                          // STR_KERNEL_BOOT_SEQUENCE
        "WARNING: Automatic Java detection enabled!",          // STR_KERNEL_JAVA_WARNING
        "KERNEL PANIC: Java contamination detected!",          // STR_KERNEL_PANIC_JAVA
        "Device manager initialized",                          // STR_DEVICE_MANAGER_INIT
        "Registered device",                                   // STR_DEVICE_REGISTERED
        "Virtual filesystem mounted",                          // STR_VFS_MOUNTED
        "System ready",                                        // STR_SYSTEM_READY
        
        "Current language:",                                   // STR_CURRENT_LANGUAGE
        "Language changed to:",                                // STR_LANGUAGE_CHANGED
        "Invalid language. Use: en, ja, zh-cn, or ko",        // STR_INVALID_LANGUAGE
        "Available languages:",                                // STR_AVAILABLE_LANGUAGES
        "Change system language",                              // STR_LANGUAGE_HELP
        
        "Input corruption detected, clearing buffers...",      // STR_INPUT_CORRUPTION
        "clearing buffers...",                                 // STR_CLEARING_BUFFERS
        "Memory error",                                        // STR_MEMORY_ERROR
        "System error",                                        // STR_SYSTEM_ERROR
        
        "Show help information",                               // STR_CMD_HELP_DESC
        "List directory contents",                             // STR_CMD_LS_DESC
        "Change directory",                                    // STR_CMD_CD_DESC
        "Display file contents",                               // STR_CMD_CAT_DESC
        "Exit the virtual machine",                            // STR_CMD_EXIT_DESC
        "Clear the screen",                                    // STR_CMD_CLEAR_DESC
        "Change system language"                               // STR_CMD_LANG_DESC
    },
    
    // Japanese translations (日本語)
    {
        "ZoraVM ヘルプシステム",                                 // STR_HELP_HEADER
        "高度な仮想マシンオペレーティングシステム",                  // STR_HELP_DESCRIPTION
        "利用可能なコマンド:",                                    // STR_HELP_AVAILABLE_COMMANDS
        "例:",                                                  // STR_HELP_EXAMPLES
        "不明なコマンド",                                        // STR_UNKNOWN_COMMAND
        "利用可能なコマンドを表示するには help と入力してください。",  // STR_TYPE_HELP
        "VMを終了中...",                                        // STR_EXITING_VM
        "ディレクトリを変更しました:",                             // STR_CHANGED_DIRECTORY
        "ディレクトリが見つかりません:",                           // STR_DIRECTORY_NOT_FOUND
        "の内容",                                              // STR_CONTENTS_OF
        "(空のディレクトリ)",                                    // STR_EMPTY_DIRECTORY
        "ファイルが見つかりません:",                              // STR_FILE_NOT_FOUND
        "ファイルを開くエラー:",                                 // STR_ERROR_OPENING_FILE
        "ファイル読み取りエラー:",                               // STR_ERROR_READING_FILE
        "アクセス拒否:",                                        // STR_PERMISSION_DENIED
        "無効な引数",                                          // STR_INVALID_ARGUMENTS
        
        "起動シーケンスを開始中...",                             // STR_KERNEL_BOOT_SEQUENCE
        "警告: 自動Java検出が有効です!",                         // STR_KERNEL_JAVA_WARNING
        "カーネルパニック: Java汚染が検出されました!",             // STR_KERNEL_PANIC_JAVA
        "デバイスマネージャーを初期化しました",                    // STR_DEVICE_MANAGER_INIT
        "デバイスを登録しました",                               // STR_DEVICE_REGISTERED
        "仮想ファイルシステムをマウントしました",                  // STR_VFS_MOUNTED
        "システム準備完了",                                    // STR_SYSTEM_READY
        
        "現在の言語:",                                         // STR_CURRENT_LANGUAGE
        "言語を変更しました:",                                  // STR_LANGUAGE_CHANGED
        "無効な言語です。en、ja、zh-cn、またはkoを使用してください", // STR_INVALID_LANGUAGE
        "利用可能な言語:",                                     // STR_AVAILABLE_LANGUAGES
        "システム言語を変更",                                  // STR_LANGUAGE_HELP
        
        "入力の破損を検出しました。バッファをクリア中...",          // STR_INPUT_CORRUPTION
        "バッファをクリア中...",                               // STR_CLEARING_BUFFERS
        "メモリエラー",                                        // STR_MEMORY_ERROR
        "システムエラー",                                      // STR_SYSTEM_ERROR
        
        "ヘルプ情報を表示",                                    // STR_CMD_HELP_DESC
        "ディレクトリの内容を一覧表示",                         // STR_CMD_LS_DESC
        "ディレクトリを変更",                                  // STR_CMD_CD_DESC
        "ファイルの内容を表示",                                // STR_CMD_CAT_DESC
        "仮想マシンを終了",                                    // STR_CMD_EXIT_DESC
        "画面をクリア",                                       // STR_CMD_CLEAR_DESC
        "システム言語を変更"                                  // STR_CMD_LANG_DESC
    },
    
    // Simplified Chinese translations (简体中文)
    {
        "ZoraVM 帮助系统",                                     // STR_HELP_HEADER
        "高级虚拟机操作系统",                                   // STR_HELP_DESCRIPTION
        "可用命令:",                                          // STR_HELP_AVAILABLE_COMMANDS
        "示例:",                                             // STR_HELP_EXAMPLES
        "未知命令",                                          // STR_UNKNOWN_COMMAND
        "输入 help 查看可用命令。",                            // STR_TYPE_HELP
        "正在退出虚拟机...",                                  // STR_EXITING_VM
        "已切换到目录:",                                      // STR_CHANGED_DIRECTORY
        "找不到目录:",                                       // STR_DIRECTORY_NOT_FOUND
        "目录内容",                                          // STR_CONTENTS_OF
        "(空目录)",                                          // STR_EMPTY_DIRECTORY
        "找不到文件:",                                       // STR_FILE_NOT_FOUND
        "打开文件错误:",                                      // STR_ERROR_OPENING_FILE
        "读取文件错误:",                                      // STR_ERROR_READING_FILE
        "权限被拒绝:",                                       // STR_PERMISSION_DENIED
        "无效参数",                                          // STR_INVALID_ARGUMENTS
        
        "正在启动引导序列...",                                // STR_KERNEL_BOOT_SEQUENCE
        "警告: 自动Java检测已启用!",                           // STR_KERNEL_JAVA_WARNING
        "内核崩溃: 检测到Java污染!",                          // STR_KERNEL_PANIC_JAVA
        "设备管理器已初始化",                                 // STR_DEVICE_MANAGER_INIT
        "已注册设备",                                        // STR_DEVICE_REGISTERED
        "虚拟文件系统已挂载",                                 // STR_VFS_MOUNTED
        "系统就绪",                                          // STR_SYSTEM_READY
        
        "当前语言:",                                         // STR_CURRENT_LANGUAGE
        "语言已更改为:",                                      // STR_LANGUAGE_CHANGED
        "无效语言。请使用: en、ja、zh-cn 或 ko",               // STR_INVALID_LANGUAGE
        "可用语言:",                                         // STR_AVAILABLE_LANGUAGES
        "更改系统语言",                                      // STR_LANGUAGE_HELP
        
        "检测到输入损坏，正在清理缓冲区...",                    // STR_INPUT_CORRUPTION
        "正在清理缓冲区...",                                 // STR_CLEARING_BUFFERS
        "内存错误",                                          // STR_MEMORY_ERROR
        "系统错误",                                          // STR_SYSTEM_ERROR
        
        "显示帮助信息",                                      // STR_CMD_HELP_DESC
        "列出目录内容",                                      // STR_CMD_LS_DESC
        "更改目录",                                          // STR_CMD_CD_DESC
        "显示文件内容",                                      // STR_CMD_CAT_DESC
        "退出虚拟机",                                        // STR_CMD_EXIT_DESC
        "清除屏幕",                                          // STR_CMD_CLEAR_DESC
        "更改系统语言"                                       // STR_CMD_LANG_DESC
    },
    
    // Korean translations (한국어)
    {
        "ZoraVM 도움말 시스템",                               // STR_HELP_HEADER
        "고급 가상머신 운영체제",                              // STR_HELP_DESCRIPTION
        "사용 가능한 명령어:",                                // STR_HELP_AVAILABLE_COMMANDS
        "예시:",                                            // STR_HELP_EXAMPLES
        "알 수 없는 명령어",                                  // STR_UNKNOWN_COMMAND
        "사용 가능한 명령어를 보려면 help를 입력하세요.",         // STR_TYPE_HELP
        "VM을 종료하는 중...",                               // STR_EXITING_VM
        "디렉토리가 변경되었습니다:",                          // STR_CHANGED_DIRECTORY
        "디렉토리를 찾을 수 없습니다:",                        // STR_DIRECTORY_NOT_FOUND
        "디렉토리 내용",                                     // STR_CONTENTS_OF
        "(빈 디렉토리)",                                     // STR_EMPTY_DIRECTORY
        "파일을 찾을 수 없습니다:",                           // STR_FILE_NOT_FOUND
        "파일 열기 오류:",                                   // STR_ERROR_OPENING_FILE
        "파일 읽기 오류:",                                   // STR_ERROR_READING_FILE
        "권한이 거부되었습니다:",                             // STR_PERMISSION_DENIED
        "잘못된 인수",                                       // STR_INVALID_ARGUMENTS
        
        "부팅 시퀀스를 시작하는 중...",                        // STR_KERNEL_BOOT_SEQUENCE
        "경고: 자동 Java 탐지가 활성화되었습니다!",             // STR_KERNEL_JAVA_WARNING
        "커널 패닉: Java 오염이 탐지되었습니다!",              // STR_KERNEL_PANIC_JAVA
        "장치 관리자가 초기화되었습니다",                      // STR_DEVICE_MANAGER_INIT
        "장치가 등록되었습니다",                              // STR_DEVICE_REGISTERED
        "가상 파일시스템이 마운트되었습니다",                   // STR_VFS_MOUNTED
        "시스템 준비 완료",                                  // STR_SYSTEM_READY
        
        "현재 언어:",                                       // STR_CURRENT_LANGUAGE
        "언어가 변경되었습니다:",                             // STR_LANGUAGE_CHANGED
        "잘못된 언어입니다. 다음을 사용하세요: en, ja, zh-cn, ko", // STR_INVALID_LANGUAGE
        "사용 가능한 언어:",                                 // STR_AVAILABLE_LANGUAGES
        "시스템 언어 변경",                                  // STR_LANGUAGE_HELP
        
        "입력 손상이 감지되었습니다. 버퍼를 정리하는 중...",      // STR_INPUT_CORRUPTION
        "버퍼를 정리하는 중...",                             // STR_CLEARING_BUFFERS
        "메모리 오류",                                       // STR_MEMORY_ERROR
        "시스템 오류",                                       // STR_SYSTEM_ERROR
        
        "도움말 정보 표시",                                  // STR_CMD_HELP_DESC
        "디렉토리 내용 나열",                                // STR_CMD_LS_DESC
        "디렉토리 변경",                                     // STR_CMD_CD_DESC
        "파일 내용 표시",                                    // STR_CMD_CAT_DESC
        "가상머신 종료",                                     // STR_CMD_EXIT_DESC
        "화면 지우기",                                       // STR_CMD_CLEAR_DESC
        "시스템 언어 변경"                                   // STR_CMD_LANG_DESC
    }
};

// Initialize i18n system
void i18n_init(void) {
    // Load saved language preference if available
    i18n_load_language_preference();
}

// Set current language
void i18n_set_language(language_t lang) {
    if (lang >= 0 && lang < ZORA_LANG_COUNT) {
        current_language = lang;
        i18n_save_language_preference();
    }
}

// Get current language
language_t i18n_get_language(void) {
    return current_language;
}

// Get localized string
const char* i18n_get_string(string_id_t id) {
    if (id >= 0 && id < STR_COUNT && current_language >= 0 && current_language < ZORA_LANG_COUNT) {
        return translations[current_language][id];
    }
    // Fallback to English if something goes wrong
    return translations[ZORA_LANG_ENGLISH][id];
}

// Get language name in English
const char* i18n_get_language_name(language_t lang) {
    if (lang >= 0 && lang < ZORA_LANG_COUNT) {
        return language_info[lang].name_english;
    }
    return "Unknown";
}

// Get language name in native script
const char* i18n_get_language_native_name(language_t lang) {
    if (lang >= 0 && lang < ZORA_LANG_COUNT) {
        return language_info[lang].name_native;
    }
    return "Unknown";
}

// Parse language string to enum
language_t i18n_parse_language(const char* lang_str) {
    if (!lang_str) return ZORA_LANG_ENGLISH;
    
    if (strcmp(lang_str, "en") == 0 || strcmp(lang_str, "english") == 0) {
        return ZORA_LANG_ENGLISH;
    } else if (strcmp(lang_str, "ja") == 0 || strcmp(lang_str, "japanese") == 0) {
        return ZORA_LANG_JAPANESE;
    } else if (strcmp(lang_str, "zh-cn") == 0 || strcmp(lang_str, "chinese") == 0 || strcmp(lang_str, "zh") == 0) {
        return ZORA_LANG_CHINESE_SIMPLIFIED;
    } else if (strcmp(lang_str, "ko") == 0 || strcmp(lang_str, "korean") == 0) {
        return ZORA_LANG_KOREAN;
    }
    
    return ZORA_LANG_COUNT; // Invalid language
}

// List all supported languages
void i18n_list_languages(void) {
    printf("%s\n", _(STR_AVAILABLE_LANGUAGES));
    for (int i = 0; i < ZORA_LANG_COUNT; i++) {
        printf("  %s - %s (%s)\n", 
               language_info[i].iso_code,
               language_info[i].name_english,
               language_info[i].name_native);
    }
}

// Save language preference to a simple config file
void i18n_save_language_preference(void) {
    FILE* config = fopen("zora_language.cfg", "w");
    if (config) {
        fprintf(config, "%d\n", current_language);
        fclose(config);
    }
}

// Load language preference from config file
void i18n_load_language_preference(void) {
    FILE* config = fopen("zora_language.cfg", "r");
    if (config) {
        int lang;
        if (fscanf(config, "%d", &lang) == 1) {
            if (lang >= 0 && lang < ZORA_LANG_COUNT) {
                current_language = lang;
            }
        }
        fclose(config);
    }
}