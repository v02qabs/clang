#define _CRT_SECURE_NO_WARNINGS
#include <curses.h>
#include <windows.h> /* For Sleep() */

#define BOARD_WIDTH  40
#define BOARD_HEIGHT 20
#define BRICK_ROWS   3
#define BRICK_COLS   38

int main(void) {
    /* --- ALL VARIABLE DECLARATIONS AT THE TOP (C89 Rule) --- */
    int paddle_x;
    int paddle_width;
    int ball_x, ball_y;
    int ball_dx, ball_dy;
    int bricks[BRICK_ROWS][BRICK_COLS];
    int score;
    int game_over;
    int ch;
    int i, j;

    /* Initialize game variables */
    paddle_x = BOARD_WIDTH / 2 - 3;
    paddle_width = 6;
    
    ball_x = BOARD_WIDTH / 2;
    ball_y = BOARD_HEIGHT - 3;
    ball_dx = 1;
    ball_dy = -1;
    
    score = 0;
    game_over = 0;

    /* Initialize bricks (1 = active, 0 = destroyed) */
    for (i = 0; i < BRICK_ROWS; i++) {
        for (j = 0; j < BRICK_COLS; j++) {
            bricks[i][j] = 1;
        }
    }

    /* --- PDCURSES INITIALIZATION --- */
    initscr();              /* Start curses mode */
    cbreak();               /* Line buffering disabled, pass on everything */
    noecho();               /* Don't echo() while do getch */
    keypad(stdscr, TRUE);   /* Enable arrow keys */
    nodelay(stdscr, TRUE);  /* Make getch() non-blocking */
    curs_set(0);            /* Hide physical cursor */

    /* --- MAIN GAME LOOP --- */
    while (!game_over) {
        /* 1. INPUT PROCESSING */
        ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case KEY_LEFT:
                    if (paddle_x > 1) {
                        paddle_x--;
                    }
                    break;
                case KEY_RIGHT:
                    if (paddle_x + paddle_width < BOARD_WIDTH - 1) {
                        paddle_x++;
                    }
                    break;
                case 'q': /* Press 'q' to quit */
                case 'Q':
                    game_over = 1;
                    break;
            }
        }

        /* 2. BALL PHYSICS & MOVEMENT */
        ball_x += ball_dx;
        ball_y += ball_dy;

        /* Wall Collisions (Left / Right) */
        if (ball_x <= 1 || ball_x >= BOARD_WIDTH - 2) {
            ball_dx = -ball_dx;
        }
        /* Wall Collision (Top) */
        if (ball_y <= 1) {
            ball_dy = -ball_dy;
        }
        /* Paddle Collision */
        if (ball_y == BOARD_HEIGHT - 2) {
            if (ball_x >= paddle_x && ball_x < paddle_x + paddle_width) {
                ball_dy = -ball_dy;
                /* Add subtle directional change based on where it hits the paddle */
                if (ball_x == paddle_x) ball_dx = -1;
                if (ball_x == paddle_x + paddle_width - 1) ball_dx = 1;
            }
        }
        /* Bottom Pit Collision (Game Over) */
        if (ball_y >= BOARD_HEIGHT - 1) {
            game_over = 1;
        }

        /* Brick Collisions */
        if (ball_y - 1 >= 0 && ball_y - 1 < BRICK_ROWS) {
            int brick_idx_y = ball_y - 1;
            int brick_idx_x = ball_x - 1;
            
            if (brick_idx_x >= 0 && brick_idx_x < BRICK_COLS) {
                if (bricks[brick_idx_y][brick_idx_x] == 1) {
                    bricks[brick_idx_y][brick_idx_x] = 0; /* Destroy brick */
                    ball_dy = -ball_dy;                  /* Bounce ball */
                    score += 10;
                }
            }
        }

        /* 3. RENDERING / DRAWING SCREEN */
        erase(); /* Clear screen before drawing frame */

        /* Draw Border Walls */
        box(stdscr, 0, 0);

        /* Draw Bricks */
        for (i = 0; i < BRICK_ROWS; i++) {
            for (j = 0; j < BRICK_COLS; j++) {
                if (bricks[i][j] == 1) {
                    mvaddch(i + 1, j + 1, '#');
                }
            }
        }

        /* Draw Paddle */
        for (i = 0; i < paddle_width; i++) {
            mvaddch(BOARD_HEIGHT - 2, paddle_x + i, '=');
        }

        /* Draw Ball */
        mvaddch(ball_y, ball_x, 'O');

        /* Draw Interface texts */
        mvprintw(BOARD_HEIGHT + 1, 2, "Score: %d", score);
        mvprintw(BOARD_HEIGHT + 2, 2, "Use LEFT/RIGHT arrow keys to move. Press 'q' to quit.");

        refresh(); /* Render to actual terminal screen */
        
        Sleep(80); /* Frame rate control (approx 12-13 FPS) */
    }

    /* --- GAME OVER SCREEN --- */
    nodelay(stdscr, FALSE); /* Stop non-blocking so user can see screen */
    clear();
    box(stdscr, 0, 0);
    mvprintw(BOARD_HEIGHT / 2, (BOARD_WIDTH / 2) - 5, "GAME OVER");
    mvprintw((BOARD_HEIGHT / 2) + 1, (BOARD_WIDTH / 2) - 7, "Final Score: %d", score);
    mvprintw((BOARD_HEIGHT / 2) + 3, (BOARD_WIDTH / 2) - 11, "Press any key to exit.");
    refresh();
    getch();

    endwin(); /* Safely close PDCurses window */
    return 0;
}