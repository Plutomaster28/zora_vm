#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unix_directories.h"
#include "vfs/vfs.h"

// Create authentic Research UNIX directory structure
int unix_directories_init(void) {
    printf("[UNIX] Initializing Research UNIX Tenth Edition directory structure...\n");
    
    if (unix_create_standard_dirs() != 0) {
        return -1;
    }
    
    if (unix_populate_manpages() != 0) {
        return -1;
    }
    
    if (unix_populate_games() != 0) {
        return -1;
    }
    
    if (unix_populate_toolchain() != 0) {
        return -1;
    }
    
    if (unix_populate_history() != 0) {
        return -1;
    }
    
    printf("[UNIX] Research UNIX directory structure initialized successfully\n");
    return 0;
}

int unix_create_standard_dirs(void) {
    const char* dirs[] = {
        UNIX_BIN,
        UNIX_USR,
        UNIX_USR_BIN,
        UNIX_USR_LIB,
        UNIX_USR_INCLUDE,
        UNIX_ETC,
        UNIX_DEV,
        UNIX_TMP,
        UNIX_VAR,
        UNIX_HOME,
        UNIX_MANDIR,
        UNIX_DOCDIR,
        UNIX_VOL2DIR,
        UNIX_GAMESDIR,
        UNIX_IPCDIR,
        UNIX_HISTORYDIR,
        UNIX_DREGSDIR,
        UNIX_SRCDIR,
        ZORA_ROOT,
        ZORA_KERNEL,
        ZORA_TOOLCHAIN,
        "/usr/man/man1",
        "/usr/man/man2",
        "/usr/man/man3",
        "/usr/man/man4",
        "/usr/man/man5",
        "/usr/man/man6",
        "/usr/man/man7",
        "/usr/man/man8",
        "/usr/src/cmd",
        "/usr/src/lib",
        "/usr/src/sys",
        "/dev/null",
        "/dev/zero",
        "/dev/tty",
        NULL
    };
    
    for (int i = 0; dirs[i] != NULL; i++) {
        if (vfs_mkdir(dirs[i]) != 0) {
            // Directory might already exist, that's OK
        }
    }
    
    return 0;
}

int unix_populate_manpages(void) {
    // Create essential man pages
    const char* man1_pages[] = {
        "ls.1", "cat.1", "cp.1", "mv.1", "rm.1", "mkdir.1", "rmdir.1",
        "chmod.1", "chown.1", "grep.1", "sed.1", "awk.1", "sort.1",
        "cc.1", "f77.1", "as.1", "ld.1", "yacc.1", "lex.1",
        "sh.1", "login.1", "passwd.1", "su.1", "who.1", "ps.1",
        "kill.1", "man.1", "help.1", "exit.1",
        NULL
    };
    
    for (int i = 0; man1_pages[i] != NULL; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/usr/man/man1/%s", man1_pages[i]);
        
        char content[1024];
        char cmd_name[32];
        strncpy(cmd_name, man1_pages[i], sizeof(cmd_name));
        char* dot = strchr(cmd_name, '.');
        if (dot) *dot = '\0';
        
        snprintf(content, sizeof(content),
            ".TH %s 1 \"ZoraVM Research UNIX\"\n"
            ".SH NAME\n"
            "%s \\- %s command\n"
            ".SH SYNOPSIS\n"
            ".B %s\n"
            "[options] [files...]\n"
            ".SH DESCRIPTION\n"
            "The\n"
            ".I %s\n"
            "command is part of the ZoraVM Research UNIX implementation.\n"
            "For detailed usage, use: %s --help\n"
            ".SH SEE ALSO\n"
            "help(1), man(1)\n",
            cmd_name, cmd_name, cmd_name, cmd_name, cmd_name, cmd_name);
        
        vfs_write_file(path, content, strlen(content));
    }
    
    return 0;
}

int unix_populate_games(void) {
    // Create classic UNIX games directory with traditional games
    const char* games[] = {
        "rogue", "adventure", "snake", "tetris", "hangman", "fortune",
        "banner", "factor", "primes", "arithmetic", "quiz", NULL
    };
    
    for (int i = 0; games[i] != NULL; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/usr/games/%s", games[i]);
        
        char content[512];
        snprintf(content, sizeof(content),
            "#!/bin/sh\n"
            "# %s - Classic UNIX game\n"
            "# Part of Research UNIX games collection\n"
            "echo \"Starting %s...\"\n"
            "echo \"This is a simulation of the classic UNIX %s game.\"\n"
            "echo \"Game implementation coming soon!\"\n",
            games[i], games[i], games[i]);
        
        vfs_write_file(path, content, strlen(content));
    }
    
    // Create fortune database
    const char* fortunes = 
        "The best way to predict the future is to implement it.\n"
        "%%\n"
        "UNIX is simple. It just takes a genius to understand its simplicity.\n"
        "%%\n"
        "In the beginning was the command line.\n"
        "%%\n"
        "ZoraVM: Because every system needs a good virtual machine.\n"
        "%%\n"
        "Real programmers use Research UNIX.\n"
        "%%\n";
    
    vfs_write_file("/usr/games/fortunes", fortunes, strlen(fortunes));
    
    return 0;
}

int unix_populate_toolchain(void) {
    // Create compiler and development tools
    const char* cc_help = 
        "#!/bin/sh\n"
        "# cc - C compiler for ZoraVM Research UNIX\n"
        "echo \"ZoraVM C Compiler v1.0\"\n"
        "echo \"Usage: cc [options] file...\"\n"
        "echo \"Options:\"\n"
        "echo \"  -c        Compile only, do not link\"\n"
        "echo \"  -o file   Output to file\"\n"
        "echo \"  -O        Optimize\"\n"
        "echo \"  -g        Generate debug info\"\n"
        "echo \"Compiler simulation - full implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/cc", cc_help, strlen(cc_help));
    
    const char* f77_help = 
        "#!/bin/sh\n"
        "# f77 - Fortran 77 compiler\n"
        "echo \"ZoraVM Fortran 77 Compiler v1.0\"\n"
        "echo \"Usage: f77 [options] file...\"\n"
        "echo \"Fortran compiler simulation - implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/f77", f77_help, strlen(f77_help));
    
    const char* as_help = 
        "#!/bin/sh\n"
        "# as - Assembler\n"
        "echo \"ZoraVM Assembler v1.0\"\n"
        "echo \"Usage: as [options] file...\"\n"
        "echo \"Assembler simulation - implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/as", as_help, strlen(as_help));
    
    const char* ld_help = 
        "#!/bin/sh\n"
        "# ld - Linker\n"
        "echo \"ZoraVM Linker v1.0\"\n"
        "echo \"Usage: ld [options] file...\"\n"
        "echo \"Linker simulation - implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/ld", ld_help, strlen(ld_help));
    
    const char* yacc_help = 
        "#!/bin/sh\n"
        "# yacc - Yet Another Compiler Compiler\n"
        "echo \"ZoraVM YACC v1.0\"\n"
        "echo \"Usage: yacc [options] file.y\"\n"
        "echo \"Parser generator simulation - implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/yacc", yacc_help, strlen(yacc_help));
    
    const char* lex_help = 
        "#!/bin/sh\n"
        "# lex - Lexical analyzer generator\n"
        "echo \"ZoraVM LEX v1.0\"\n"
        "echo \"Usage: lex [options] file.l\"\n"
        "echo \"Lexical analyzer simulation - implementation coming soon!\"\n";
    
    vfs_write_file("/usr/bin/lex", lex_help, strlen(lex_help));
    
    return 0;
}

int unix_populate_history(void) {
    // Create version and history information
    const char* version_info = 
        "ZoraVM Research UNIX Implementation\n"
        "Version: 10th Edition (Zora Flavor)\n"
        "Kernel: ZORA v2.1.0\n"
        "Shell: MERL (Modern Enhanced Research Language)\n"
        "Built: " __DATE__ " " __TIME__ "\n"
        "\n"
        "Based on Research UNIX principles with modern enhancements:\n"
        "- Complete virtual machine environment\n"
        "- Sandboxed execution with resource limits\n"
        "- Multi-language scripting support\n"
        "- Virtual networking and IPC simulation\n"
        "- Modern terminal compatibility\n"
        "\n"
        "ZoraVM brings the elegance of Research UNIX to the modern world.\n";
    
    vfs_write_file(ZORA_VERSION, version_info, strlen(version_info));
    
    const char* changelog = 
        "ZoraVM Research UNIX Changelog\n"
        "==============================\n"
        "\n"
        "Version 2.1.0 (Sakemono Release)\n"
        "- Complete Research UNIX directory structure\n"
        "- Traditional UNIX games collection\n"
        "- Compiler toolchain simulation\n"
        "- Enhanced man page system\n"
        "- IPC and debugging tools\n"
        "- Historical authenticity improvements\n"
        "\n"
        "Version 2.0.0\n"
        "- Terminal compatibility system\n"
        "- Windows Terminal support\n"
        "- System monitoring suite\n"
        "- Professional kernel boot\n"
        "\n"
        "Version 1.0.0\n"
        "- Initial ZoraVM implementation\n"
        "- MERL shell with 80+ commands\n"
        "- Virtual filesystem\n"
        "- Sandbox security system\n";
    
    vfs_write_file(ZORA_CHANGELOG, changelog, strlen(changelog));
    
    // Create passwd and group files
    const char* passwd_content = 
        "root:x:0:0:root:/root:/bin/sh\n"
        "daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin\n"
        "bin:x:2:2:bin:/bin:/usr/sbin/nologin\n"
        "sys:x:3:3:sys:/dev:/usr/sbin/nologin\n"
        "sync:x:4:65534:sync:/bin:/bin/sync\n"
        "games:x:5:60:games:/usr/games:/usr/sbin/nologin\n"
        "man:x:6:12:man:/var/cache/man:/usr/sbin/nologin\n"
        "lp:x:7:7:lp:/var/spool/lpd:/usr/sbin/nologin\n"
        "mail:x:8:8:mail:/var/mail:/usr/sbin/nologin\n"
        "news:x:9:9:news:/var/spool/news:/usr/sbin/nologin\n"
        "uucp:x:10:10:uucp:/var/spool/uucp:/usr/sbin/nologin\n"
        "proxy:x:13:13:proxy:/bin:/usr/sbin/nologin\n"
        "www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin\n"
        "backup:x:34:34:backup:/var/backups:/usr/sbin/nologin\n"
        "list:x:38:38:Mailing List Manager:/var/list:/usr/sbin/nologin\n"
        "nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin\n";
    
    vfs_write_file("/etc/passwd", passwd_content, strlen(passwd_content));
    
    const char* group_content = 
        "root:x:0:\n"
        "daemon:x:1:\n"
        "bin:x:2:\n"
        "sys:x:3:\n"
        "adm:x:4:\n"
        "tty:x:5:\n"
        "disk:x:6:\n"
        "lp:x:7:\n"
        "mail:x:8:\n"
        "news:x:9:\n"
        "uucp:x:10:\n"
        "man:x:12:\n"
        "proxy:x:13:\n"
        "kmem:x:15:\n"
        "dialout:x:20:\n"
        "fax:x:21:\n"
        "voice:x:22:\n"
        "cdrom:x:24:\n"
        "floppy:x:25:\n"
        "tape:x:26:\n"
        "sudo:x:27:\n"
        "audio:x:29:\n"
        "dip:x:30:\n"
        "www-data:x:33:\n"
        "backup:x:34:\n"
        "operator:x:37:\n"
        "list:x:38:\n"
        "src:x:40:\n"
        "gnats:x:41:\n"
        "shadow:x:42:\n"
        "utmp:x:43:\n"
        "video:x:44:\n"
        "sasl:x:45:\n"
        "plugdev:x:46:\n"
        "staff:x:50:\n"
        "games:x:60:\n"
        "users:x:100:\n"
        "nogroup:x:65534:\n";
    
    vfs_write_file("/etc/group", group_content, strlen(group_content));
    
    return 0;
}

void unix_directories_cleanup(void) {
    // Cleanup handled by VFS
}
