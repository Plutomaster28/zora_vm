#ifndef UNIX_GAMES_H
#define UNIX_GAMES_H

// Research UNIX Games System
// Classic UNIX games collection for authentic experience

#include <stddef.h>

// Game types
typedef enum {
    GAME_ROGUE,
    GAME_ADVENTURE,
    GAME_SNAKE,
    GAME_TETRIS,
    GAME_HANGMAN,
    GAME_FORTUNE,
    GAME_BANNER,
    GAME_ARITHMETIC,
    GAME_FACTOR,
    GAME_PRIMES
} GameType;

// Game state structure
typedef struct {
    GameType type;
    int running;
    int score;
    int level;
    char player_name[32];
    time_t start_time;
    int moves;
} GameState;

// Function prototypes
int unix_games_init(void);
void unix_games_cleanup(void);

// Individual games
int unix_play_rogue(void);
int unix_play_adventure(void);
int unix_play_snake(void);
int unix_play_tetris(void);
int unix_play_hangman(void);
int unix_show_fortune(void);
int unix_make_banner(const char* text);
int unix_arithmetic_quiz(void);
int unix_factor_number(int number);
int unix_generate_primes(int limit);

// Game utilities
void unix_list_games(void);
int unix_launch_game(const char* game_name);
void unix_show_game_help(const char* game_name);
void unix_show_high_scores(void);

// Fortune system
int unix_add_fortune(const char* fortune);
const char* unix_get_random_fortune(void);
int unix_load_fortunes(void);

#endif // UNIX_GAMES_H
