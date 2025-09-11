#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif
#include "unix_games_snake.h"

static SnakeGame current_game;

#ifdef _WIN32
void clear_screen() {
    system("cls");
}

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hide_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void show_cursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}
#else
void clear_screen() {
    printf("\033[2J\033[H");
}

void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

void hide_cursor() {
    printf("\033[?25l");
}

void show_cursor() {
    printf("\033[?25h");
}
#endif

int snake_game_init(SnakeGame* game) {
    memset(game, 0, sizeof(SnakeGame));
    
    // Initialize snake in center
    game->snake.length = 3;
    game->snake.body[0].x = BOARD_WIDTH / 2;
    game->snake.body[0].y = BOARD_HEIGHT / 2;
    game->snake.body[1].x = BOARD_WIDTH / 2 - 1;
    game->snake.body[1].y = BOARD_HEIGHT / 2;
    game->snake.body[2].x = BOARD_WIDTH / 2 - 2;
    game->snake.body[2].y = BOARD_HEIGHT / 2;
    
    game->snake.direction = 1; // Moving right
    game->snake.score = 0;
    game->snake.alive = 1;
    game->game_over = 0;
    game->high_score = 0;
    
    // Clear board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            game->board[y][x] = ' ';
        }
    }
    
    snake_generate_food(game);
    return 0;
}

void snake_generate_food(SnakeGame* game) {
    int valid_position = 0;
    while (!valid_position) {
        game->food.x = rand() % BOARD_WIDTH;
        game->food.y = rand() % BOARD_HEIGHT;
        
        // Check if food is not on snake
        valid_position = 1;
        for (int i = 0; i < game->snake.length; i++) {
            if (game->snake.body[i].x == game->food.x && 
                game->snake.body[i].y == game->food.y) {
                valid_position = 0;
                break;
            }
        }
    }
}

void snake_draw_board(SnakeGame* game) {
    clear_screen();
    
    // Draw title and score
    gotoxy(0, 0);
    printf(" ZoraVM Snake Game - Score: %d | High Score: %d", 
           game->snake.score, game->high_score);
    
    gotoxy(0, 1);
    printf("Use WASD to move, Q to quit");
    
    // Draw top border
    gotoxy(0, 3);
    printf("┌");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("─");
    printf("┐");
    
    // Draw game area
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        gotoxy(0, y + 4);
        printf("│");
        
        for (int x = 0; x < BOARD_WIDTH; x++) {
            // Check if this position has snake body
            int is_snake = 0;
            for (int i = 0; i < game->snake.length; i++) {
                if (game->snake.body[i].x == x && game->snake.body[i].y == y) {
                    if (i == 0) {
                        printf("O"); // Head
                    } else {
                        printf("o"); // Body
                    }
                    is_snake = 1;
                    break;
                }
            }
            
            if (!is_snake) {
                // Check if this is food
                if (game->food.x == x && game->food.y == y) {
                    printf("*");
                } else {
                    printf(" ");
                }
            }
        }
        printf("│");
    }
    
    // Draw bottom border
    gotoxy(0, BOARD_HEIGHT + 4);
    printf("└");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("─");
    printf("┘");
    
    // Instructions
    gotoxy(0, BOARD_HEIGHT + 6);
    printf("Controls: W=Up, A=Left, S=Down, D=Right, Q=Quit");
}

int snake_get_input(void) {
#ifdef _WIN32
    if (_kbhit()) {
        return _getch();
    }
    return 0;
#else
    int ch = 0;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    ch = getchar();
    fcntl(STDIN_FILENO, F_SETFL, flags);
    return ch;
#endif
}

void snake_move_snake(SnakeGame* game) {
    // Move body segments
    for (int i = game->snake.length - 1; i > 0; i--) {
        game->snake.body[i] = game->snake.body[i - 1];
    }
    
    // Move head based on direction
    switch (game->snake.direction) {
        case 0: // Up
            game->snake.body[0].y--;
            break;
        case 1: // Right
            game->snake.body[0].x++;
            break;
        case 2: // Down
            game->snake.body[0].y++;
            break;
        case 3: // Left
            game->snake.body[0].x--;
            break;
    }
}

int snake_check_collision(SnakeGame* game) {
    Position head = game->snake.body[0];
    
    // Check wall collision
    if (head.x < 0 || head.x >= BOARD_WIDTH || 
        head.y < 0 || head.y >= BOARD_HEIGHT) {
        return 1;
    }
    
    // Check self collision
    for (int i = 1; i < game->snake.length; i++) {
        if (head.x == game->snake.body[i].x && 
            head.y == game->snake.body[i].y) {
            return 1;
        }
    }
    
    return 0;
}

void snake_update_game(SnakeGame* game) {
    snake_move_snake(game);
    
    // Check collisions
    if (snake_check_collision(game)) {
        game->snake.alive = 0;
        game->game_over = 1;
        return;
    }
    
    // Check food collision
    if (game->snake.body[0].x == game->food.x && 
        game->snake.body[0].y == game->food.y) {
        
        // Grow snake
        game->snake.length++;
        game->snake.score += 10;
        
        // Update high score
        if (game->snake.score > game->high_score) {
            game->high_score = game->snake.score;
        }
        
        // Generate new food
        snake_generate_food(game);
    }
}

int snake_game_run(void) {
    srand(time(NULL));
    snake_game_init(&current_game);
    
    hide_cursor();
    
    printf(" ZoraVM Snake Game\n");
    printf("====================\n\n");
    printf("Get ready! Game starts in 3 seconds...\n");
    printf("Use WASD to control the snake!\n");
    
#ifdef _WIN32
    Sleep(3000);
#else
    sleep(3);
#endif
    
    while (!current_game.game_over) {
        snake_draw_board(&current_game);
        
        // Handle input
        int key = snake_get_input();
        if (key) {
            switch (tolower(key)) {
                case 'w':
                    if (current_game.snake.direction != 2) // Can't go down if going up
                        current_game.snake.direction = 0;
                    break;
                case 'd':
                    if (current_game.snake.direction != 3) // Can't go left if going right
                        current_game.snake.direction = 1;
                    break;
                case 's':
                    if (current_game.snake.direction != 0) // Can't go up if going down
                        current_game.snake.direction = 2;
                    break;
                case 'a':
                    if (current_game.snake.direction != 1) // Can't go right if going left
                        current_game.snake.direction = 3;
                    break;
                case 'q':
                    current_game.game_over = 1;
                    break;
            }
        }
        
        snake_update_game(&current_game);
        
        // Game speed (smaller = faster)
#ifdef _WIN32
        Sleep(150);
#else
        usleep(150000);
#endif
    }
    
    // Game over screen
    clear_screen();
    printf(" Game Over!\n");
    printf("=============\n\n");
    
    if (current_game.snake.alive) {
        printf("Thanks for playing!\n");
    } else {
        printf(" You crashed!\n");
    }
    
    printf("Final Score: %d\n", current_game.snake.score);
    printf("High Score: %d\n", current_game.high_score);
    printf("Snake Length: %d\n", current_game.snake.length);
    
    show_cursor();
    
    printf("\nPress any key to continue...");
#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
    
    return 0;
}

void snake_cleanup(SnakeGame* game) {
    show_cursor();
    memset(game, 0, sizeof(SnakeGame));
}
