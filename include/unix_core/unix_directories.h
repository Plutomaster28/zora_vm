#ifndef UNIX_DIRECTORIES_H
#define UNIX_DIRECTORIES_H

// Research UNIX Tenth Edition directory structure for ZoraVM
// Authentic UNIX experience with Zora flavor

// Standard UNIX directories
#define UNIX_ROOT           "/"
#define UNIX_BIN            "/bin"
#define UNIX_USR            "/usr"
#define UNIX_USR_BIN        "/usr/bin"
#define UNIX_USR_LIB        "/usr/lib"
#define UNIX_USR_INCLUDE    "/usr/include"
#define UNIX_ETC            "/etc"
#define UNIX_DEV            "/dev"
#define UNIX_TMP            "/tmp"
#define UNIX_VAR            "/var"
#define UNIX_HOME           "/home"

// Research UNIX specific directories
#define UNIX_MANDIR         "/usr/man"
#define UNIX_DOCDIR         "/usr/doc"
#define UNIX_VOL2DIR        "/usr/vol2"
#define UNIX_GAMESDIR       "/usr/games"
#define UNIX_IPCDIR         "/usr/ipc"
#define UNIX_HISTORYDIR     "/usr/history"
#define UNIX_DREGSDIR       "/usr/dregs"
#define UNIX_SRCDIR         "/usr/src"

// Zora-specific additions
#define ZORA_ROOT           "/zora"
#define ZORA_VERSION        "/zora/version"
#define ZORA_CHANGELOG      "/zora/changelog"
#define ZORA_KERNEL         "/zora/kernel"
#define ZORA_TOOLCHAIN      "/zora/toolchain"

// Compiler and toolchain directories
#define UNIX_CC             "/usr/bin/cc"
#define UNIX_FORTRAN        "/usr/bin/f77"
#define UNIX_ASM            "/usr/bin/as"
#define UNIX_LD             "/usr/bin/ld"
#define UNIX_YACC           "/usr/bin/yacc"
#define UNIX_LEX            "/usr/bin/lex"
#define UNIX_DBX            "/usr/bin/dbx"
#define UNIX_SDB            "/usr/bin/sdb"

// Text processing tools
#define UNIX_TROFF          "/usr/bin/troff"
#define UNIX_NROFF          "/usr/bin/nroff"
#define UNIX_AWK            "/usr/bin/awk"
#define UNIX_SED            "/usr/bin/sed"

// Function prototypes
int unix_directories_init(void);
int unix_create_standard_dirs(void);
int unix_populate_manpages(void);
int unix_populate_games(void);
int unix_populate_toolchain(void);
int unix_populate_history(void);
void unix_directories_cleanup(void);

#endif // UNIX_DIRECTORIES_H
