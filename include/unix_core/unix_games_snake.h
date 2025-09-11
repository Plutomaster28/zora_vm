#ifndef UNIX_GAMES_SNAKE_H
#define UNIX_GAMES_SNAKE_H

// Snake game implementation
#define BOARD_WIDTH 40
#define BOARD_HEIGHT 20
#define MAX_SNAKE_LENGTH 400

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position body[MAX_SNAKE_LENGTH];
    int length;
    int direction; // 0=up, 1=right, 2=down, 3=left
    int score;
    int alive;
} Snake;

typedef struct {
    Position food;
    char board[BOARD_HEIGHT][BOARD_WIDTH];
    Snake snake;
    int game_over;
    int high_score;
} SnakeGame;

// Snake game functions
int snake_game_init(SnakeGame* game);
int snake_game_run(void);
void snake_draw_board(SnakeGame* game);
void snake_update_game(SnakeGame* game);
int snake_check_collision(SnakeGame* game);
void snake_generate_food(SnakeGame* game);
void snake_move_snake(SnakeGame* game);
int snake_get_input(void);
void snake_cleanup(SnakeGame* game);

#endif // UNIX_GAMES_SNAKE_H
