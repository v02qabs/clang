//#include <crypto/compiler.h> /* Standard fallback for bcc32c setup if needed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#define MAX_ROWS 100
#define MAX_COLS 80
#define FILENAME_MAX_LEN 260

/* Global state strictly managed for C89 compatibility */
char text_buffer[MAX_ROWS][MAX_COLS];
int total_lines = 0;
char current_filename[FILENAME_MAX_LEN] = "";

/* Function Declarations */
void init_editor(void);
void draw_ui(int cursor_row, int cursor_col);
void handle_load(void);
void handle_save(void);
void handle_save_as(void);

void init_editor(void) {
    int i;
    for (i = 0; i < MAX_ROWS; i++) {
        text_buffer[i][0] = '\0';
    }
    total_lines = 1;
}

void draw_ui(int cursor_row, int cursor_col) {
    int i;
    /* Clear and refresh the status top-bar */
    move(0, 0);
    clrtoeol();
    attron(A_REVERSE);
    printw(" File: %s | F2: Load | F3: Save | F4: Save As | F10: Quit", 
           current_filename[0] == '\0' ? "[Untitled]" : current_filename);
    attroff(A_REVERSE);

    /* Draw the text buffer */
    for (i = 0; i < MAX_ROWS; i++) {
        if (i < total_lines || text_buffer[i][0] != '\0') {
            move(i + 2, 0);
            clrtoeol();
            printw("%s", text_buffer[i]);
        }
    }

    /* Keep the cursor at the editing position */
    move(cursor_row + 2, cursor_col);
    refresh();
}

void handle_load(void) {
    char input_file[FILENAME_MAX_LEN];
    FILE *fp;
    int row;
    
    /* Input dialog layout at the bottom of the screen */
    move(LINES - 1, 0);
    clrtoeol();
    printw("Load File Path: ");
    echo();
    getnstr(input_file, FILENAME_MAX_LEN - 1);
    noecho();

    if (input_file[0] == '\0') return;

    fp = fopen(input_file, "r");
    if (fp == NULL) {
        move(LINES - 1, 0);
        clrtoeol();
        printw("Error: Could not open file! Press any key...");
        getch();
        return;
    }

    /* Reset buffer and load content */
    init_editor();
    row = 0;
    while (row < MAX_ROWS && fgets(text_buffer[row], MAX_COLS, fp) != NULL) {
        /* Strip trailing newline character */
        size_t len = strlen(text_buffer[row]);
        if (len > 0 && text_buffer[row][len - 1] == '\n') {
            text_buffer[row][len - 1] = '\0';
        }
        row++;
    }
    total_lines = (row == 0) ? 1 : row;
    strncpy(current_filename, input_file, FILENAME_MAX_LEN - 1);
    fclose(fp);
}

void handle_save(void) {
    FILE *fp;
    int i;

    /* If no file is associated, route directly to Save As */
    if (current_filename[0] == '\0') {
        handle_save_as();
        return;
    }

    fp = fopen(current_filename, "w");
    if (fp == NULL) {
        move(LINES - 1, 0);
        clrtoeol();
        printw("Error: Could not save file! Press any key...");
        getch();
        return;
    }

    for (i = 0; i < total_lines; i++) {
        fprintf(fp, "%s\n", text_buffer[i]);
    }
    fclose(fp);

    move(LINES - 1, 0);
    clrtoeol();
    printw("File saved successfully! Press any key...");
    getch();
}

void handle_save_as(void) {
    char input_file[FILENAME_MAX_LEN];
    
    move(LINES - 1, 0);
    clrtoeol();
    printw("Save As File Path: ");
    echo();
    getnstr(input_file, FILENAME_MAX_LEN - 1);
    noecho();

    if (input_file[0] != '\0') {
        strncpy(current_filename, input_file, FILENAME_MAX_LEN - 1);
        handle_save();
    }
}

int main(void) {
    int ch;
    int cursor_row = 0;
    int cursor_col = 0;
    int quit_flag = 0;
    size_t len;

    /* Initialize PDCurses interface */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    init_editor();

    /* Core Application Game/Interaction Loop */
    while (!quit_flag) {
        draw_ui(cursor_row, cursor_col);
        ch = getch();

        switch (ch) {
            case KEY_F(2): /* LOAD */
                handle_load();
                cursor_row = 0;
                cursor_col = 0;
                break;

            case KEY_F(3): /* SAVE */
                handle_save();
                break;

            case KEY_F(4): /* SAVE AS */
                handle_save_as();
                break;

            case KEY_F(10): /* QUIT */
                quit_flag = 1;
                break;

            case KEY_UP:
                if (cursor_row > 0) {
                    cursor_row--;
                    if (cursor_col > (int)strlen(text_buffer[cursor_row])) {
                        cursor_col = strlen(text_buffer[cursor_row]);
                    }
                }
                break;

            case KEY_DOWN:
                if (cursor_row < total_lines - 1) {
                    cursor_row++;
                    if (cursor_col > (int)strlen(text_buffer[cursor_row])) {
                        cursor_col = strlen(text_buffer[cursor_row]);
                    }
                }
                break;

            case KEY_LEFT:
                if (cursor_col > 0) {
                    cursor_col--;
                } else if (cursor_row > 0) {
                    cursor_row--;
                    cursor_col = strlen(text_buffer[cursor_row]);
                }
                break;

            case KEY_RIGHT:
                if (cursor_col < (int)strlen(text_buffer[cursor_row])) {
                    cursor_col++;
                } else if (cursor_row < total_lines - 1) {
                    cursor_row++;
                    cursor_col = 0;
                }
                break;

            case KEY_BACKSPACE:
            case 8: /* Backspace literal */
                if (cursor_col > 0) {
                    /* Shift characters left to delete */
                    memmove(&text_buffer[cursor_row][cursor_col - 1], 
                            &text_buffer[cursor_row][cursor_col], 
                            strlen(&text_buffer[cursor_row][cursor_col]) + 1);
                    cursor_col--;
                }
                break;

            case '\n':
            case '\r': /* Enter Key split line */
                if (total_lines < MAX_ROWS) {
                    /* Shift text below down */
                    int r;
                    for (r = total_lines; r > cursor_row + 1; r--) {
                        strcpy(text_buffer[r], text_buffer[r - 1]);
                    }
                    /* Move right part of the line to a new row */
                    strcpy(text_buffer[cursor_row + 1], &text_buffer[cursor_row][cursor_col]);
                    text_buffer[cursor_row][cursor_col] = '\0';
                    
                    total_lines++;
                    cursor_row++;
                    cursor_col = 0;
                }
                break;

            default:
                /* Handle normal character entry */
                len = strlen(text_buffer[cursor_row]);
                if (ch >= 32 && ch <= 126 && (int)len < MAX_COLS - 1) {
                    /* Shift text right to insert character */
                    memmove(&text_buffer[cursor_row][cursor_col + 1], 
                            &text_buffer[cursor_row][cursor_col], 
                            strlen(&text_buffer[cursor_row][cursor_col]) + 1);
                    text_buffer[cursor_row][cursor_col] = (char)ch;
                    cursor_col++;
                }
                break;
        }
    }

    endwin();
    return 0;
}