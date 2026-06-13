#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <wininet.h>
#include <curses.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILES 200
#define MAX_NAME_LEN 260

int main(int argc, char* argv[]) {
    /* C89 Rule: All variables declared at the very top of the block */
    HINTERNET hInternet;
    HINTERNET hFtpSession;
    WIN32_FIND_DATAA findData;
    HINTERNET hFind;
    BOOL bNext;
    
    char file_list[MAX_FILES][MAX_NAME_LEN];
    int file_count;
    int current_selection;
    int ch;
    int i;
    int start_row;
    
    char* ftp_server;
    char* ftp_user;
    char* ftp_pass;

    /* Validate command-line parameters first */
    if (argc < 4) {
        printf("Error: Missing parameters.\n\n");
        printf("Usage:\n");
        printf("  %s <server> <username> <password>\n\n", argv[0]);
        printf("Example:\n");
        printf("  %s ftp.example.com myuser mysecretpass\n", argv[0]);
        return 1;
    }

    /* Assign arguments to configuration pointers */
    ftp_server = argv[1];
    ftp_user   = argv[2];
    ftp_pass   = argv[3];

    /* Initialize engine defaults */
    hInternet = NULL;
    hFtpSession = NULL;
    hFind = NULL;
    bNext = TRUE;
    file_count = 0;
    current_selection = 0;
    ch = 0;
    start_row = 3;

    /* Initialize Curses TUI environment */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); 

    mvprintw(0, 1, "=== bcc32c + PDCurses WinINet FTP Client ===");
    mvprintw(1, 1, "Connecting to %s...", ftp_server);
    refresh();

    /* Establish network environment handle */
    hInternet = InternetOpenA("Bcc32cFtpClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        mvprintw(2, 1, "InternetOpenA failed. Press any key to quit.");
        getch();
        endwin();
        return 1;
    }

    /* Authenticate against the remote server using parameters */
    hFtpSession = InternetConnectA(hInternet, ftp_server, INTERNET_INVALID_PORT_NUMBER,
                                   ftp_user, ftp_pass, INTERNET_SERVICE_FTP, 0, 0);
    if (hFtpSession == NULL) {
        InternetCloseHandle(hInternet);
        mvprintw(2, 1, "Authentication failed. Check credentials. Press any key.");
        getch();
        endwin();
        return 1;
    }

    /* Read target directory structure */
    hFind = FtpFindFirstFileA(hFtpSession, "*.*", &findData, 0, 0);
    if (hFind != NULL) {
        while (bNext && file_count < MAX_FILES) {
            strncpy(file_list[file_count], findData.cFileName, MAX_NAME_LEN - 1);
            file_list[file_count][MAX_NAME_LEN - 1] = '\0';
            file_count++;
            bNext = InternetFindNextFileA(hFind, &findData);
        }
        InternetCloseHandle(hFind);
    }

    /* UI Navigation Loop */
    while (1) {
        move(start_row, 0);
        clrtobot();

        if (file_count == 0) {
            mvprintw(start_row, 2, "No files found or access denied.");
        } else {
            for (i = 0; i < file_count; i++) {
                if (i == current_selection) {
                    attron(A_REVERSE); 
                    mvprintw(start_row + i, 2, " > %s ", file_list[i]);
                    attroff(A_REVERSE);
                } else {
                    mvprintw(start_row + i, 4, "%s", file_list[i]);
                }
            }
        }

        mvprintw(LINES - 2, 1, "Use UP/DOWN arrows to highlight files.");
        mvprintw(LINES - 1, 1, "Press 'q' to disconnect safely and exit.");
        refresh();

        ch = getch();

        if (ch == 'q' || ch == 'Q') {
            break;
        }

        switch (ch) {
            case KEY_UP:
                if (current_selection > 0) {
                    current_selection--;
                }
                break;
            case KEY_DOWN:
                if (current_selection < file_count - 1) {
                    current_selection++;
                }
                break;
            default:
                break;
        }
    }

    /* Graceful cleanup */
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    endwin();
    return 0;
}