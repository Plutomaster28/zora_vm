#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif
#include "unix_games.h"
#include "unix_games_snake.h"
#include "unix_games_hangman.h"
#include "vfs/vfs.h"
#include "terminal/terminal_detector.h"

static char fortunes[100][256];
static int fortune_count = 0;
static int games_initialized = 0;

int unix_games_init(void) {
    if (games_initialized) return 0;
    
    printf("[GAMES] Initializing Research UNIX Games Collection...\n");
    
    // Create games directory structure
    vfs_mkdir("/usr/games");
    vfs_mkdir("/usr/games/lib");
    vfs_mkdir("/usr/games/scores");
    
    // Load fortunes
    unix_load_fortunes();
    
    // Create game executables
    const char* rogue_script = 
        "#!/bin/sh\n"
        "# Rogue - The classic dungeon adventure\n"
        "echo \"Welcome to Rogue!\"\n"
        "echo \"Explore the dungeon, fight monsters, collect treasure!\"\n"
        "echo \"Use WASD to move, 'q' to quit\"\n"
        "rogue-game\n";
    
    vfs_write_file("/usr/games/rogue", rogue_script, strlen(rogue_script));
    
    const char* adventure_script = 
        "#!/bin/sh\n"
        "# Adventure - Text adventure game\n"
        "echo \"Welcome to Adventure!\"\n"
        "echo \"You are standing at the end of a road before a small brick building.\"\n"
        "echo \"Around you is a forest. A small stream flows out of the building.\"\n"
        "adventure-game\n";
    
    vfs_write_file("/usr/games/adventure", adventure_script, strlen(adventure_script));
    
    games_initialized = 1;
    printf("[GAMES] Games collection initialized\n");
    return 0;
}

int unix_load_fortunes(void) {
    fortune_count = 0;
    
    // Load built-in fortunes
    strcpy(fortunes[fortune_count++], "The best way to predict the future is to implement it.");
    strcpy(fortunes[fortune_count++], "UNIX is simple. It just takes a genius to understand its simplicity.");
    strcpy(fortunes[fortune_count++], "In the beginning was the command line.");
    strcpy(fortunes[fortune_count++], "ZoraVM: Because every system needs a good virtual machine.");
    strcpy(fortunes[fortune_count++], "Real programmers use Research UNIX.");
    strcpy(fortunes[fortune_count++], "There are only 10 types of people: those who understand binary and those who don't.");
    strcpy(fortunes[fortune_count++], "It's not a bug, it's an undocumented feature.");
    strcpy(fortunes[fortune_count++], "Code never lies, comments sometimes do.");
    strcpy(fortunes[fortune_count++], "The most likely way for the world to be destroyed is by accident. That's where we come in; we're computer programmers, we create accidents.");
    strcpy(fortunes[fortune_count++], "Programming is like sex: one mistake and you have to support it for the rest of your life.");
    
    return fortune_count;
}

const char* unix_get_random_fortune(void) {
    if (fortune_count == 0) {
        return "Fortune database is empty.";
    }
    
    srand(time(NULL));
    int index = rand() % fortune_count;
    return fortunes[index];
}

int unix_show_fortune(void) {
    printf("Fortune says:\n");
    printf("\n");
    
    const char* tl = get_box_char(BOX_TOP_LEFT);
    const char* tr = get_box_char(BOX_TOP_RIGHT);
    const char* bl = get_box_char(BOX_BOTTOM_LEFT);
    const char* br = get_box_char(BOX_BOTTOM_RIGHT);
    const char* h = get_box_char(BOX_HORIZONTAL);
    const char* v = get_box_char(BOX_VERTICAL);
    
    const char* fortune = unix_get_random_fortune();
    int len = strlen(fortune);
    int width = len > 60 ? 60 : len + 4;
    
    // Top border
    printf("%s", tl);
    for (int i = 0; i < width; i++) printf("%s", h);
    printf("%s\n", tr);
    
    // Content
    printf("%s ", v);
    
    // Word wrap the fortune
    const char* p = fortune;
    int line_len = 0;
    while (*p) {
        if (*p == ' ' && line_len > 50) {
            printf(" %s\n%s ", v, v);
            line_len = 0;
        } else {
            putchar(*p);
            line_len++;
        }
        p++;
    }
    
    // Pad to width
    for (int i = line_len; i < width - 1; i++) {
        putchar(' ');
    }
    printf("%s\n", v);
    
    // Bottom border
    printf("%s", bl);
    for (int i = 0; i < width; i++) printf("%s", h);
    printf("%s\n", br);
    
    return 0;
}

int unix_make_banner(const char* text) {
    printf("ZoraVM Banner v1.0\n");
    printf("\n");
    
    // Simple ASCII art banner
    int len = strlen(text);
    
    // Top border
    for (int i = 0; i < len + 4; i++) printf("*");
    printf("\n");
    
    // Text with border
    printf("* %s *\n", text);
    
    // Bottom border
    for (int i = 0; i < len + 4; i++) printf("*");
    printf("\n");
    
    return 0;
}

int unix_play_snake(void) {
    printf("🐍 Starting ZoraVM Snake Game...\n");
    printf("===============================\n");
    return snake_game_run();
}

int unix_play_hangman(void) {
    printf("🎯 Starting ZoraVM Hangman Game...\n");
    printf("==================================\n");
    return hangman_game_run();
}

int unix_arithmetic_quiz(void) {
    printf("🧮 ZoraVM Interactive Arithmetic Quiz\n");
    printf("====================================\n");
    printf("\n");
    
    srand(time(NULL));
    int score = 0;
    int questions = 10;
    int difficulty = 1;
    
    printf("Welcome to the interactive arithmetic quiz!\n");
    printf("Choose difficulty level:\n");
    printf("1. Easy (1-20)\n");
    printf("2. Medium (1-50)\n");
    printf("3. Hard (1-100)\n");
    printf("Enter choice (1-3): ");
    
    char input[10];
    if (fgets(input, sizeof(input), stdin)) {
        difficulty = atoi(input);
        if (difficulty < 1 || difficulty > 3) difficulty = 1;
    }
    
    int range = (difficulty == 1) ? 20 : (difficulty == 2) ? 50 : 100;
    printf("\nDifficulty: %s (numbers 1-%d)\n", 
           (difficulty == 1) ? "Easy" : (difficulty == 2) ? "Medium" : "Hard", range);
    printf("Answer %d questions correctly for a perfect score!\n\n", questions);
    
    for (int i = 0; i < questions; i++) {
        int a = rand() % range + 1;
        int b = rand() % range + 1;
        int operation = rand() % 4;
        int answer, correct;
        char operator;
        
        printf("Question %d/%d: ", i + 1, questions);
        
        switch (operation) {
            case 0:
                correct = a + b;
                operator = '+';
                break;
            case 1:
                if (a < b) { int temp = a; a = b; b = temp; } // Ensure positive result
                correct = a - b;
                operator = '-';
                break;
            case 2:
                // Keep numbers smaller for multiplication
                a = rand() % (range/2) + 1;
                b = rand() % (range/2) + 1;
                correct = a * b;
                operator = '*';
                break;
            case 3:
                // Division: ensure clean division
                correct = rand() % range + 1;
                b = rand() % 10 + 1;
                a = correct * b;
                operator = '/';
                break;
        }
        
        printf("%d %c %d = ? ", a, operator, b);
        
        if (fgets(input, sizeof(input), stdin)) {
            answer = atoi(input);
            
            if (answer == correct) {
                printf("✅ Correct!\n");
                score++;
            } else {
                printf("❌ Wrong! The answer is %d\n", correct);
            }
        } else {
            printf("❌ Invalid input! The answer is %d\n", correct);
        }
        printf("\n");
    }
    
    printf("🎯 Quiz Complete!\n");
    printf("================\n");
    printf("Final Score: %d/%d (%.1f%%)\n", score, questions, (float)score/questions * 100);
    
    if (score == questions) {
        printf("🏆 Perfect score! You're a math wizard!\n");
    } else if (score >= questions * 0.8) {
        printf("🌟 Excellent work! Great math skills!\n");
    } else if (score >= questions * 0.6) {
        printf("👍 Good job! Keep practicing!\n");
    } else {
        printf("📚 Keep studying! Practice makes perfect!\n");
    }
    
    return 0;
}

int unix_factor_number(int number) {
    printf("ZoraVM Factor v1.0\n");
    printf("Prime factorization of %d:\n", number);
    
    if (number <= 1) {
        printf("Number must be greater than 1\n");
        return 1;
    }
    
    printf("%d =", number);
    
    int n = number;
    int first = 1;
    
    // Factor out 2s
    while (n % 2 == 0) {
        if (!first) printf(" *");
        printf(" 2");
        first = 0;
        n /= 2;
    }
    
    // Factor out odd numbers
    for (int i = 3; i * i <= n; i += 2) {
        while (n % i == 0) {
            if (!first) printf(" *");
            printf(" %d", i);
            first = 0;
            n /= i;
        }
    }
    
    // If n is still > 1, it's a prime
    if (n > 1) {
        if (!first) printf(" *");
        printf(" %d", n);
    }
    
    printf("\n");
    return 0;
}

int unix_generate_primes(int limit) {
    printf("ZoraVM Primes v1.0\n");
    printf("Prime numbers up to %d:\n\n", limit);
    
    if (limit < 2) {
        printf("No primes less than 2\n");
        return 0;
    }
    
    // Sieve of Eratosthenes
    char* is_prime = malloc(limit + 1);
    if (!is_prime) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    memset(is_prime, 1, limit + 1);
    is_prime[0] = is_prime[1] = 0;
    
    for (int i = 2; i * i <= limit; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= limit; j += i) {
                is_prime[j] = 0;
            }
        }
    }
    
    int count = 0;
    for (int i = 2; i <= limit; i++) {
        if (is_prime[i]) {
            printf("%4d", i);
            count++;
            if (count % 10 == 0) printf("\n");
        }
    }
    
    if (count % 10 != 0) printf("\n");
    printf("\nTotal primes found: %d\n", count);
    
    free(is_prime);
    return 0;
}

void unix_list_games(void) {
    printf("🎮 ZoraVM Research UNIX Games Collection\n");
    printf("========================================\n");
    printf("\n");
    printf("🕹️  INTERACTIVE GAMES:\n");
    printf("  snake       - 🐍 Real Snake game (WASD controls)\n");
    printf("  hangman     - 🎯 Interactive word guessing game\n");
    printf("  tetris      - 🧩 Block puzzle game (coming soon)\n");
    printf("  arithmetic  - 🧮 Interactive math quiz\n");
    printf("\n");
    printf("🎨 UTILITIES & DEMOS:\n");
    printf("  fortune     - 🔮 Display random fortune\n");
    printf("  banner      - 🎨 Create ASCII art banners\n");
    printf("  factor      - 🔢 Prime factorization tool\n");
    printf("  primes      - 📊 Generate prime numbers\n");
    printf("\n");
    printf("📚 CLASSIC UNIX GAMES:\n");
    printf("  rogue       - ⚔️  Classic dungeon adventure (coming soon)\n");
    printf("  adventure   - 📖 Text-based adventure game (coming soon)\n");
    printf("\n");
    printf("Usage: games <game_name> [options]\n");
    printf("       games --list     (show this list)\n");
    printf("       games --help <game_name>\n");
    printf("\n");
    printf("💡 Tip: Try 'snake' or 'hangman' for fully interactive games!\n");
}

int unix_launch_game(const char* game_name) {
    if (strcmp(game_name, "fortune") == 0) {
        return unix_show_fortune();
    } else if (strcmp(game_name, "snake") == 0) {
        return unix_play_snake();
    } else if (strcmp(game_name, "hangman") == 0) {
        return unix_play_hangman();
    } else if (strcmp(game_name, "arithmetic") == 0) {
        return unix_arithmetic_quiz();
    } else if (strcmp(game_name, "factor") == 0) {
        return unix_factor_number(12345);  // Example
    } else if (strcmp(game_name, "primes") == 0) {
        return unix_generate_primes(100);  // Example
    } else if (strncmp(game_name, "banner", 6) == 0) {
        return unix_make_banner("ZoraVM Research UNIX");
    } else {
        printf("Game '%s' not implemented yet\n", game_name);
        printf("Available games: fortune, snake, hangman, arithmetic, factor, primes, banner\n");
        return 1;
    }
}

void unix_games_cleanup(void) {
    games_initialized = 0;
}
