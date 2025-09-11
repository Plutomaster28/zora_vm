#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#endif
#include "unix_games_hangman.h"

static const char* word_list[] = {
    "COMPUTER", "PROGRAMMING", "UNIX", "ZORA", "VIRTUAL", "MACHINE",
    "KERNEL", "SHELL", "TERMINAL", "DEBUGGING", "COMPILER", "ASSEMBLY",
    "FORTRAN", "PYTHON", "JAVASCRIPT", "ALGORITHMS", "DATABASE",
    "NETWORK", "SECURITY", "ENCRYPTION", "PROTOCOL", "INTERFACE",
    "ARCHITECTURE", "DEVELOPMENT", "SOFTWARE", "HARDWARE", "MEMORY",
    "PROCESSOR", "GRAPHICS", "AUDIO", "VIDEO", "GAMING", "SIMULATION",
    "VIRTUAL", "REALITY", "ARTIFICIAL", "INTELLIGENCE", "MACHINE",
    "LEARNING", "NEURAL", "ROBOTICS", "QUANTUM", "BLOCKCHAIN",
    "CYBERSECURITY", "ALGORITHM", "STRUCTURE", "FUNCTION", "VARIABLE",
    "CONSTANT", "POINTER", "REFERENCE"
};

static HangmanGame current_game;

void hangman_get_random_word(char* word) {
    int word_count = sizeof(word_list) / sizeof(word_list[0]);
    int index = rand() % word_count;
    strcpy(word, word_list[index]);
}

int hangman_game_init(HangmanGame* game) {
    memset(game, 0, sizeof(HangmanGame));
    
    hangman_get_random_word(game->word);
    game->word_length = strlen(game->word);
    
    // Initialize guessed word with underscores
    for (int i = 0; i < game->word_length; i++) {
        if (game->word[i] == ' ') {
            game->guessed[i] = ' ';
        } else {
            game->guessed[i] = '_';
        }
    }
    game->guessed[game->word_length] = '\0';
    
    game->wrong_count = 0;
    game->solved = 0;
    game->game_over = 0;
    game->guess_count = 0;
    
    return 0;
}

void hangman_draw_gallows(int wrong_count) {
    printf("   +---+\n");
    printf("   |   |\n");
    
    switch (wrong_count) {
        case 0:
            printf("       |\n");
            printf("       |\n");
            printf("       |\n");
            printf("       |\n");
            break;
        case 1:
            printf("   O   |\n");
            printf("       |\n");
            printf("       |\n");
            printf("       |\n");
            break;
        case 2:
            printf("   O   |\n");
            printf("   |   |\n");
            printf("       |\n");
            printf("       |\n");
            break;
        case 3:
            printf("   O   |\n");
            printf("  /|   |\n");
            printf("       |\n");
            printf("       |\n");
            break;
        case 4:
            printf("   O   |\n");
            printf("  /|\\  |\n");
            printf("       |\n");
            printf("       |\n");
            break;
        case 5:
            printf("   O   |\n");
            printf("  /|\\  |\n");
            printf("  /    |\n");
            printf("       |\n");
            break;
        case 6:
            printf("   O   |\n");
            printf("  /|\\  |\n");
            printf("  / \\  |\n");
            printf("       |\n");
            break;
    }
    printf("=========\n");
}

void hangman_display_word(const char* guessed, int length) {
    printf("\nWord: ");
    for (int i = 0; i < length; i++) {
        printf("%c ", guessed[i]);
    }
    printf("\n");
}

int hangman_letter_already_guessed(const HangmanGame* game, char letter) {
    for (int i = 0; i < game->guess_count; i++) {
        if (game->previous_guesses[i] == letter) {
            return 1;
        }
    }
    return 0;
}

int hangman_check_letter(HangmanGame* game, char letter) {
    letter = toupper(letter);
    
    // Check if already guessed
    if (hangman_letter_already_guessed(game, letter)) {
        return -1; // Already guessed
    }
    
    // Add to previous guesses
    game->previous_guesses[game->guess_count++] = letter;
    
    int found = 0;
    
    // Check if letter is in word
    for (int i = 0; i < game->word_length; i++) {
        if (game->word[i] == letter) {
            game->guessed[i] = letter;
            found = 1;
        }
    }
    
    if (!found) {
        game->wrong_letters[game->wrong_count] = letter;
        game->wrong_count++;
        
        if (game->wrong_count >= MAX_GUESSES) {
            game->game_over = 1;
        }
    }
    
    // Check if word is complete
    if (hangman_is_word_complete(game->guessed, game->word_length)) {
        game->solved = 1;
        game->game_over = 1;
    }
    
    return found;
}

int hangman_is_word_complete(const char* guessed, int length) {
    for (int i = 0; i < length; i++) {
        if (guessed[i] == '_') {
            return 0;
        }
    }
    return 1;
}

int hangman_game_run(void) {
    srand(time(NULL));
    hangman_game_init(&current_game);
    
    printf(" ZoraVM Hangman Game\n");
    printf("======================\n\n");
    printf("Welcome to Hangman! Guess the word letter by letter.\n");
    printf("You have %d wrong guesses before the game ends.\n\n", MAX_GUESSES);
    
    while (!current_game.game_over) {
        // Clear screen and display game state
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        
        printf(" ZoraVM Hangman Game\n");
        printf("======================\n\n");
        
        hangman_draw_gallows(current_game.wrong_count);
        hangman_display_word(current_game.guessed, current_game.word_length);
        
        printf("\nWrong guesses (%d/%d): ", current_game.wrong_count, MAX_GUESSES);
        for (int i = 0; i < current_game.wrong_count; i++) {
            printf("%c ", current_game.wrong_letters[i]);
        }
        printf("\n");
        
        printf("Previous guesses: ");
        for (int i = 0; i < current_game.guess_count; i++) {
            printf("%c ", current_game.previous_guesses[i]);
        }
        printf("\n\n");
        
        // Get player input
        printf("Enter a letter (or 'quit' to exit): ");
        char input[10];
        if (fgets(input, sizeof(input), stdin)) {
            // Remove newline
            input[strcspn(input, "\\n")] = 0;
            
            if (strcmp(input, "quit") == 0) {
                printf("Thanks for playing!\n");
                return 0;
            }
            
            if (strlen(input) != 1 || !isalpha(input[0])) {
                printf("Please enter a single letter.\n");
                printf("Press Enter to continue...");
                getchar();
                continue;
            }
            
            int result = hangman_check_letter(&current_game, input[0]);
            
            if (result == -1) {
                printf("You already guessed '%c'!\n", toupper(input[0]));
                printf("Press Enter to continue...");
                getchar();
            } else if (result == 1) {
                printf("Good guess! '%c' is in the word.\n", toupper(input[0]));
                if (!current_game.game_over) {
                    printf("Press Enter to continue...");
                    getchar();
                }
            } else {
                printf("Sorry, '%c' is not in the word.\n", toupper(input[0]));
                printf("Press Enter to continue...");
                getchar();
            }
        }
    }
    
    // Game over screen
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    
    printf(" ZoraVM Hangman - Game Over!\n");
    printf("===============================\n\n");
    
    hangman_draw_gallows(current_game.wrong_count);
    
    if (current_game.solved) {
        printf(" Congratulations! You won!\n");
        printf("You guessed the word: %s\n", current_game.word);
        printf("Wrong guesses: %d/%d\n", current_game.wrong_count, MAX_GUESSES);
    } else {
        printf(" You lost! The man was hanged.\n");
        printf("The word was: %s\n", current_game.word);
        printf("Better luck next time!\n");
    }
    
    printf("\nTotal guesses made: %d\n", current_game.guess_count);
    printf("Efficiency: %.1f%%\n", 
           (float)(current_game.word_length) / current_game.guess_count * 100);
    
    printf("\nPress Enter to continue...");
    getchar();
    
    return 0;
}
