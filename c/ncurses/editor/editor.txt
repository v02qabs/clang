#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 256

char text_buffer[MAX_LINES][MAX_LINE_LENGTH];
int num_lines = 0;
int current_line = 0;
int current_col = 0;
int offset_y = 0;
int offset_x = 0;

void editor_init();
void editor_draw();
void editor_process_keypress();
void editor_save();
void editor_open(const char* filename);

/*
 * Initialize the ncurses screen and settings.
 */
void editor_init() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    clear();
}

/*
 * Draw the current state of the text buffer to the screen.
 */
void editor_draw() {
    clear();

    // Calculate dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Draw lines
    for (int i = 0; i < max_y - 1; i++) {
        int line_index = offset_y + i;
        if (line_index < num_lines) {
            mvprintw(i, 0, "%s", text_buffer[line_index] + offset_x);
        }
    }

    // Draw status bar
    mvprintw(max_y - 1, 0, "Line: %d/%d, Col: %d/%d | Press Ctrl-S to save, Ctrl-Q to quit",
             current_line + 1, num_lines, current_col + 1, (int)strlen(text_buffer[current_line]));

    move(current_line - offset_y, current_col - offset_x);
    refresh();
}

/*
 * Process a single key press from the user.
 */
void editor_process_keypress() {
    int ch = getch();

    switch (ch) {
        case KEY_UP:
            if (current_line > 0) {
                current_line--;
                if (current_col > strlen(text_buffer[current_line])) {
                    current_col = strlen(text_buffer[current_line]);
                }
            }
            break;
        case KEY_DOWN:
            if (current_line < num_lines - 1) {
                current_line++;
                if (current_col > strlen(text_buffer[current_line])) {
                    current_col = strlen(text_buffer[current_line]);
                }
            }
            break;
        case KEY_LEFT:
            if (current_col > 0) {
                current_col--;
            } else if (current_line > 0) {
                current_line--;
                current_col = strlen(text_buffer[current_line]);
            }
            break;
        case KEY_RIGHT:
            if (current_col < strlen(text_buffer[current_line])) {
                current_col++;
            } else if (current_line < num_lines - 1) {
                current_line++;
                current_col = 0;
            }
            break;
        case '\n':
            // Handle new line
            if (num_lines < MAX_LINES) {
                // Shift existing lines down
                for (int i = num_lines; i > current_line + 1; i--) {
                    strcpy(text_buffer[i], text_buffer[i-1]);
                }
                
                // Copy rest of current line to new line
                strcpy(text_buffer[current_line + 1], text_buffer[current_line] + current_col);
                
                // Truncate current line
                text_buffer[current_line][current_col] = '\0';
                
                current_line++;
                current_col = 0;
                num_lines++;
            }
            break;
        case KEY_BACKSPACE:
            // Handle backspace
            if (current_col > 0) {
                memmove(text_buffer[current_line] + current_col - 1,
                        text_buffer[current_line] + current_col,
                        strlen(text_buffer[current_line]) - current_col + 1);
                current_col--;
            } else if (current_line > 0) {
                // Merge current line with previous line
                int prev_len = strlen(text_buffer[current_line - 1]);
                strcat(text_buffer[current_line - 1], text_buffer[current_line]);
                
                // Shift subsequent lines up
                for (int i = current_line; i < num_lines; i++) {
                    strcpy(text_buffer[i], text_buffer[i+1]);
                }
                num_lines--;
                current_line--;
                current_col = prev_len;
            }
            break;
        case 19: // Ctrl-S
            editor_save();
            break;
        case 17: // Ctrl-Q
            endwin();
            exit(0);
        default:
            // Handle normal characters
            if (ch >= 32 && ch < 127) { // Printable characters
                if (current_col < MAX_LINE_LENGTH - 1) {
                    memmove(text_buffer[current_line] + current_col + 1,
                            text_buffer[current_line] + current_col,
                            strlen(text_buffer[current_line]) - current_col + 1);
                    text_buffer[current_line][current_col] = (char)ch;
                    current_col++;
                }
            }
            break;
    }

    // Adjust screen offsets to keep cursor in view
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    if (current_line < offset_y) offset_y = current_line;
    if (current_line >= offset_y + max_y - 1) offset_y = current_line - (max_y - 2);
    if (current_col < offset_x) offset_x = current_col;
    if (current_col >= offset_x + max_x) offset_x = current_col - (max_x - 1);
}

/*
 * Save the current text buffer to a file.
 */
void editor_save() {
    char filename[256];
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    mvprintw(max_y - 1, 0, "Save as: ");
    echo();
    getstr(filename);
    noecho();

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        mvprintw(max_y - 1, 0, "Error: Could not save file!");
        refresh();
        getch();
        return;
    }

    for (int i = 0; i < num_lines; i++) {
        fprintf(fp, "%s\n", text_buffer[i]);
    }

    fclose(fp);
    mvprintw(max_y - 1, 0, "File saved successfully!");
    refresh();
    getch();
}

/*
 * Open a file and load its contents into the text buffer.
 */
void editor_open(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        // File doesn't exist, start with an empty buffer
        strcpy(text_buffer[0], "");
        num_lines = 1;
        return;
    }
    
    char line[MAX_LINE_LENGTH];
    num_lines = 0;
    while (fgets(line, sizeof(line), fp) != NULL && num_lines < MAX_LINES) {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        strcpy(text_buffer[num_lines], line);
        num_lines++;
    }

    fclose(fp);
    if (num_lines == 0) {
        strcpy(text_buffer[0], "");
        num_lines = 1;
    }
}

int main(int argc, char* argv[]) {
    editor_init();

    if (argc > 1) {
        editor_open(argv[1]);
    } else {
        strcpy(text_buffer[0], "");
        num_lines = 1;
    }
    
    while (1) {
        editor_draw();
        editor_process_keypress();
    }

    endwin();
    return 0;
}
