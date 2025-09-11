#ifndef UNIX_GAMES_HANGMAN_H
#define UNIX_GAMES_HANGMAN_H

#define MAX_WORD_LENGTH 32
#define MAX_GUESSES 6
#define MAX_WORDS 50

typedef struct {
    char word[MAX_WORD_LENGTH];
    char guessed[MAX_WORD_LENGTH];
    char wrong_letters[26];
    int wrong_count;
    int word_length;
    int solved;
    int game_over;
    char previous_guesses[26];
    int guess_count;
} HangmanGame;

// Hangman game functions
int hangman_game_init(HangmanGame* game);
int hangman_game_run(void);
void hangman_draw_gallows(int wrong_count);
void hangman_display_word(const char* guessed, int length);
int hangman_check_letter(HangmanGame* game, char letter);
int hangman_is_word_complete(const char* guessed, int length);
void hangman_get_random_word(char* word);
int hangman_letter_already_guessed(const HangmanGame* game, char letter);

#endif // UNIX_GAMES_HANGMAN_H
