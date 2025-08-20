#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>

#define WIDTH 10
#define HEIGHT 20
#define BLOCK_SIZE 1

// Tetromino shapes (4 rotations each)
const int tetrominoes[7][4][4][2] = {
    // I
    {{{0,0},{1,0},{2,0},{3,0}}, {{0,0},{0,1},{0,2},{0,3}}, {{0,0},{1,0},{2,0},{3,0}}, {{0,0},{0,1},{0,2},{0,3}}},
    // O
    {{{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}},
    // T
    {{{0,0},{1,0},{2,0},{1,1}}, {{0,0},{0,1},{0,2},{1,1}}, {{0,1},{1,1},{2,1},{1,0}}, {{0,1},{1,0},{1,1},{1,2}}},
    // S
    {{{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}}, {{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}}},
    // Z
    {{{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}}, {{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}}},
    // J
    {{{0,0},{1,0},{2,0},{2,1}}, {{0,0},{0,1},{1,1},{2,1}}, {{0,0},{0,1},{1,0},{2,0}}, {{0,0},{1,0},{0,1},{0,2}}},
    // L
    {{{0,1},{1,1},{2,1},{2,0}}, {{0,0},{1,0},{2,0},{2,1}}, {{0,0},{0,1},{1,1},{2,1}}, {{0,0},{0,1},{1,0},{2,0}}}
};

int board[HEIGHT][WIDTH] = {0};
int current_x, current_y, current_type, current_rotation;
int score = 0;
int level = 1;
int lines_cleared = 0;
int game_over = 0;

// Function prototypes
void init_game();
void spawn_piece();
void draw_board();
int check_collision(int dx, int dy, int rot);
void place_piece();
void clear_lines();
void handle_input();
void drop_piece();
int get_drop_delay();

int main() {
    srand(time(NULL));
    init_game();
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);  // Non-blocking input
    curs_set(0); // Hide cursor

    while (!game_over) {
        draw_board();
        handle_input();
        static int counter = 0;
        if (++counter >= get_drop_delay()) {
            drop_piece();
            counter = 0;
        }
        usleep(10000);  // 10ms sleep for responsiveness
    }

    endwin();
    printf("Game Over! Score: %d\n", score);
    return 0;
}

void init_game() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = 0;
        }
    }
    spawn_piece();
}

void spawn_piece() {
    current_type = rand() % 7;
    current_rotation = 0;
    current_x = WIDTH / 2 - 2;
    current_y = 0;
    if (check_collision(0, 0, 0)) {
        game_over = 1;
    }
}

void draw_board() {
    clear();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (board[y][x]) {
                mvprintw(y, x * 2, "##");
            } else {
                mvprintw(y, x * 2, "..");
            }
        }
    }
    // Draw current piece
    for (int i = 0; i < 4; i++) {
        int px = current_x + tetrominoes[current_type][current_rotation][i][0];
        int py = current_y + tetrominoes[current_type][current_rotation][i][1];
        if (py >= 0 && py < HEIGHT && px >= 0 && px < WIDTH) {
            mvprintw(py, px * 2, "##");
        }
    }
    // Draw borders
    for (int y = 0; y <= HEIGHT; y++) {
        mvprintw(y, WIDTH * 2, "|");
        mvprintw(y, -1, "|");
    }
    for (int x = -1; x <= WIDTH; x++) {
        mvprintw(HEIGHT, x * 2, "--");
    }
    // Draw score and level
    mvprintw(0, WIDTH * 2 + 2, "Score: %d", score);
    mvprintw(1, WIDTH * 2 + 2, "Level: %d", level);
    mvprintw(2, WIDTH * 2 + 2, "Lines: %d", lines_cleared);
    refresh();
}

int check_collision(int dx, int dy, int rot) {
    int new_rot = (current_rotation + rot + 4) % 4;
    for (int i = 0; i < 4; i++) {
        int px = current_x + dx + tetrominoes[current_type][new_rot][i][0];
        int py = current_y + dy + tetrominoes[current_type][new_rot][i][1];
        if (px < 0 || px >= WIDTH || py >= HEIGHT || (py >= 0 && board[py][px])) {
            return 1;
        }
    }
    return 0;
}

void place_piece() {
    for (int i = 0; i < 4; i++) {
        int px = current_x + tetrominoes[current_type][current_rotation][i][0];
        int py = current_y + tetrominoes[current_type][current_rotation][i][1];
        if (py >= 0) {
            board[py][px] = 1;
        }
    }
    clear_lines();
    spawn_piece();
}

void clear_lines() {
    int lines = 0;
    for (int y = HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < WIDTH; x++) {
            if (!board[y][x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            lines++;
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < WIDTH; x++) {
                    board[yy][x] = board[yy - 1][x];
                }
            }
            for (int x = 0; x < WIDTH; x++) {
                board[0][x] = 0;
            }
            y++;  // Re-check the same line after shift
        }
    }
    if (lines > 0) {
        score += (lines * lines) * 100 * level;
        lines_cleared += lines;
        level = 1 + lines_cleared / 10;
    }
}

void handle_input() {
    int ch = getch();
    switch (ch) {
        case KEY_LEFT:
            if (!check_collision(-1, 0, 0)) current_x--;
            break;
        case KEY_RIGHT:
            if (!check_collision(1, 0, 0)) current_x++;
            break;
        case KEY_DOWN:
            if (!check_collision(0, 1, 0)) current_y++;
            break;
        case KEY_UP:
            if (!check_collision(0, 0, 1)) current_rotation = (current_rotation + 1) % 4;
            break;
        case ' ':
            while (!check_collision(0, 1, 0)) current_y++;
            place_piece();
            break;
        case 'q':
            game_over = 1;
            break;
    }
}

void drop_piece() {
    if (!check_collision(0, 1, 0)) {
        current_y++;
    } else {
        place_piece();
    }
}

int get_drop_delay() {
    // Delay in loop iterations (each ~10ms), so 100 = 1s
    return 100 / level;
}
