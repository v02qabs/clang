#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define BYTES_PER_ROW 16
#define VIEW_ROWS 16

void dump_page(unsigned char *buffer, size_t size, size_t offset, int cursor_pos) {
    clear();
    mvprintw(0, 0, " bcc32c + PDCurses Hex Editor | File Size: %zu bytes", size);
    mvprintw(1, 0, "-------------------------------------------------------------------------------");
    mvprintw(2, 0, " Offset    00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  Decoded Text");
    mvprintw(3, 0, "-------------------------------------------------------------------------------");

    for (int r = 0; r < VIEW_ROWS; r++) {
        size_t row_offset = offset + (r * BYTES_PER_ROW);
        if (row_offset >= size) break;

        // 1. Print Offset
        mvprintw(4 + r, 0, " %08X ", (unsigned int)row_offset);

        // 2. Print Hex values
        for (int c = 0; c < BYTES_PER_ROW; c++) {
            size_t byte_idx = row_offset + c;
            int screen_col = 11 + (c * 3) + (c >= 8 ? 1 : 0);

            if (byte_idx < size) {
                // Highlight cursor position
                if ((int)(byte_idx - offset) == cursor_pos) {
                    attron(A_REVERSE);
                    mvprintw(4 + r, screen_col, "%02X", buffer[byte_idx]);
                    attroff(A_REVERSE);
                } else {
                    mvprintw(4 + r, screen_col, "%02X", buffer[byte_idx]);
                }
            } else {
                mvprintw(4 + r, screen_col, "  ");
            }
        }

        // 3. Print ASCII text representation
        mvprintw(4 + r, 61, "| ");
        for (int c = 0; c < BYTES_PER_ROW; c++) {
            size_t byte_idx = row_offset + c;
            if (byte_idx < size) {
                unsigned char ch = buffer[byte_idx];
                // Display printable characters, otherwise print a dot
                if (ch < 32 || ch > 126) ch = '.';
                
                if ((int)(byte_idx - offset) == cursor_pos) {
                    attron(A_REVERSE);
                    mvprintw(4 + r, 63 + c, "%c", ch);
                    attroff(A_REVERSE);
                } else {
                    mvprintw(4 + r, 63 + c, "%c", ch);
                }
            }
        }
    }

    mvprintw(21, 0, "-------------------------------------------------------------------------------");
    mvprintw(22, 0, " [Arrows] Navigate   [Q] Quit");
    refresh();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Load file into memory
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    if (file_size == 0) {
        printf("File is empty.\n");
        fclose(file);
        return 1;
    }
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, file);
    fclose(file);

    // Initialize PDCurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0); // Hide hardware cursor

    size_t page_offset = 0;
    int cursor_pos = 0; // Relative to page_offset
    int max_visible_bytes = VIEW_ROWS * BYTES_PER_ROW;

    int ch;
    while (1) {
        dump_page(buffer, file_size, page_offset, cursor_pos);

        ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        size_t current_global_pos = page_offset + cursor_pos;

        switch (ch) {
            case KEY_LEFT:
                if (cursor_pos > 0) {
                    cursor_pos--;
                } else if (page_offset > 0) {
                    page_offset -= BYTES_PER_ROW;
                    cursor_pos += BYTES_PER_ROW - 1;
                }
                break;

            case KEY_RIGHT:
                if (current_global_pos < file_size - 1) {
                    if (cursor_pos < max_visible_bytes - 1) {
                        cursor_pos++;
                    } else {
                        page_offset += BYTES_PER_ROW;
                    }
                }
                break;

            case KEY_UP:
                if (cursor_pos >= BYTES_PER_ROW) {
                    cursor_pos -= BYTES_PER_ROW;
                } else if (page_offset >= BYTES_PER_ROW) {
                    page_offset -= BYTES_PER_ROW;
                }
                break;

            case KEY_DOWN:
                if (current_global_pos + BYTES_PER_ROW < file_size) {
                    if (cursor_pos + BYTES_PER_ROW < max_visible_bytes) {
                        cursor_pos += BYTES_PER_ROW;
                    } else {
                        page_offset += BYTES_PER_ROW;
                    }
                }
                break;
        }
    }

    // Clean up
    endwin();
    free(buffer);
    return 0;
}