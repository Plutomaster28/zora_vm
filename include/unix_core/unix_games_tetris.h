#ifndef UNIX_GAMES_TETRIS_H
#define UNIX_GAMES_TETRIS_H

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define PIECE_SIZE 4

typedef struct {
    int shape[PIECE_SIZE][PIECE_SIZE];
    int x, y;
    int type;
    int rotation;
} TetrisPiece;

typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    TetrisPiece current_piece;
    TetrisPiece next_piece;
    int score;
    int level;
    int lines_cleared;
    int game_over;
    int drop_timer;
    int drop_speed;
} TetrisGame;

// Tetris game functions
int tetris_game_init(TetrisGame* game);
int tetris_game_run(void);
void tetris_draw_board(TetrisGame* game);
void tetris_spawn_piece(TetrisGame* game);
int tetris_can_move(TetrisGame* game, int dx, int dy, int rotation);
void tetris_move_piece(TetrisGame* game, int dx, int dy);
void tetris_rotate_piece(TetrisGame* game);
void tetris_place_piece(TetrisGame* game);
int tetris_clear_lines(TetrisGame* game);
void tetris_update_game(TetrisGame* game);
int tetris_get_input(void);

#endif // UNIX_GAMES_TETRIS_H
