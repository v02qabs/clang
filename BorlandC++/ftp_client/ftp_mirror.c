#include <windows.h>
#include <wininet.h>
#include <curses.h>
#include <string.h>
#include <stdio.h>

#define MAX_FILES 500
#define BUFFER_SIZE 512

/* ファイル比較用の同期データ構造体 (C89準拠) */
typedef struct {
    char file_name[MAX_PATH];
    DWORD file_size;
    FILETIME last_modified;
} SyncFileInfo;

/* グローバルバッファ */
static char status_msg[BUFFER_SIZE] = "Welcome to FTP Mirroring Client! [U: Mirror Upload | D: Mirror Download | Q: Quit]";

/* 画面描画関数（接続情報を上部に表示するよう拡張） */
void DrawInterface(const char* srv, const char* usr, const char* r_dir, const char* l_dir) {
    clear();
    box(stdscr, 0, 0);
    mvprintw(1, 2, "==========================================================================");
    mvprintw(2, 2, "      FTP MIRRORING CLIENT (bcc32c + PDCurses + WinINet)                  ");
    mvprintw(3, 2, "==========================================================================");
    
    /* コマンドラインから受け取ったターゲット情報を表示 */
    mvprintw(5, 4, "FTP Server : %s", srv);
    mvprintw(6, 4, "FTP User   : %s", usr);
    mvprintw(7, 4, "Remote Dir : %s", r_dir);
    mvprintw(8, 4, "Local Dir  : %s", l_dir);
    mvprintw(9, 2, "--------------------------------------------------------------------------");

    mvprintw(11, 4, "[Press 'U'] -> Mirroring Upload   (Local  ==> Remote)");
    mvprintw(12, 4, "[Press 'D'] -> Mirroring Download (Remote ==> Local)");
    mvprintw(13, 4, "[Press 'Q'] -> Quit Application");
    
    mvprintw(16, 4, "Status:");
    mvprintw(17, 4, "%s", status_msg);
    mvprintw(19, 2, "==========================================================================");
    refresh();
}

/* タイムスタンプ（更新日時）を比較する関数 */
int IsFileNewer(FILETIME ft1, FILETIME ft2) {
    if (ft1.dwHighDateTime > ft2.dwHighDateTime) return 1;
    if (ft1.dwHighDateTime < ft2.dwHighDateTime) return 0;
    return (ft1.dwLowDateTime > ft2.dwLowDateTime) ? 1 : 0;
}

/* 1. ミラーリング・ダウンロード関数 (Remote ==> Local) */
void PerformMirroringDownload(HINTERNET hFtpSession, const char* remote_dir, const char* local_dir) {
    SyncFileInfo remote_list[MAX_FILES];
    int remote_count;
    int i;
    HINTERNET hFind;
    WIN32_FIND_DATAA find_data;
    
    int skip_count;
    int up_to_date_count;
    int download_count;
    
    remote_count = 0;
    skip_count = 0;
    up_to_date_count = 0;
    download_count = 0;

    if (!FtpSetCurrentDirectoryA(hFtpSession, remote_dir)) {
        sprintf(status_msg, "Error: Remote directory not found.");
        return;
    }
    if (!SetCurrentDirectoryA(local_dir)) {
        sprintf(status_msg, "Error: Local directory not found.");
        return;
    }

    hFind = FtpFindFirstFileA(hFtpSession, "*.*", &find_data, 0, 0);
    if (hFind != NULL) {
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strcpy(remote_list[remote_count].file_name, find_data.cFileName);
                remote_list[remote_count].file_size = find_data.nFileSizeLow;
                remote_list[remote_count].last_modified = find_data.ftLastWriteTime;
                remote_count++;
            }
        } while (InternetFindNextFileA(hFind, &find_data) && remote_count < MAX_FILES);
        InternetCloseHandle(hFind);
    }

    for (i = 0; i < remote_count; i++) {
        HANDLE hLocalFile;
        WIN32_FIND_DATAA local_find;
        BOOL bExists;
        BOOL bNeedDownload;

        bExists = FALSE;
        bNeedDownload = FALSE;

        hLocalFile = FindFirstFileA(remote_list[i].file_name, &local_find);
        if (hLocalFile != INVALID_HANDLE_VALUE) {
            bExists = TRUE;
            FindClose(hLocalFile);
        }

        if (!bExists) {
            bNeedDownload = TRUE;
        } else {
            if (local_find.nFileSizeLow != remote_list[i].file_size || 
                IsFileNewer(remote_list[i].last_modified, local_find.ftLastWriteTime)) {
                bNeedDownload = TRUE;
            } else {
                up_to_date_count++;
            }
        }

        if (bNeedDownload) {
            if (FtpGetFileA(hFtpSession, remote_list[i].file_name, remote_list[i].file_name, FALSE, 
                            FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
                
                HANDLE hFileToTouch = CreateFileA(remote_list[i].file_name, GENERIC_WRITE, 0, NULL, 
                                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFileToTouch != INVALID_HANDLE_VALUE) {
                    SetFileTime(hFileToTouch, NULL, NULL, &(remote_list[i].last_modified));
                    CloseHandle(hFileToTouch);
                }
                download_count++;
            } else {
                skip_count++;
            }
        }
    }

    sprintf(status_msg, "Mirror Down Complete. Downloaded: %d, Skipped: %d, Failed: %d", 
            download_count, up_to_date_count, skip_count);
}

/* 2. ミラーリング・アップロード関数 (Local ==> Remote) */
void PerformMirroringUpload(HINTERNET hFtpSession, const char* remote_dir, const char* local_dir) {
    SyncFileInfo local_list[MAX_FILES];
    int local_count;
    int i;
    HANDLE hFind;
    WIN32_FIND_DATAA find_data;

    int skip_count;
    int up_to_date_count;
    int upload_count;

    local_count = 0;
    skip_count = 0;
    up_to_date_count = 0;
    upload_count = 0;

    if (!FtpSetCurrentDirectoryA(hFtpSession, remote_dir)) {
        sprintf(status_msg, "Error: Remote directory not found.");
        return;
    }
    if (!SetCurrentDirectoryA(local_dir)) {
        sprintf(status_msg, "Error: Local directory not found.");
        return;
    }

    hFind = FindFirstFileA("*.*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strcpy(local_list[local_count].file_name, find_data.cFileName);
                local_list[local_count].file_size = find_data.nFileSizeLow;
                local_list[local_count].last_modified = find_data.ftLastWriteTime;
                local_count++;
            }
        } while (FindNextFileA(hFind, &find_data) && local_count < MAX_FILES);
        FindClose(hFind);
    }

    for (i = 0; i < local_count; i++) {
        HINTERNET hRemoteFind;
        WIN32_FIND_DATAA remote_find;
        BOOL bExists;
        BOOL bNeedUpload;

        bExists = FALSE;
        bNeedUpload = FALSE;

        hRemoteFind = FtpFindFirstFileA(hFtpSession, local_list[i].file_name, &remote_find, 0, 0);
        if (hRemoteFind != NULL) {
            bExists = TRUE;
            InternetCloseHandle(hRemoteFind);
        }

        if (!bExists) {
            bNeedUpload = TRUE;
        } else {
            if (remote_find.nFileSizeLow != local_list[i].file_size ||
                IsFileNewer(local_list[i].last_modified, remote_find.ftLastWriteTime)) {
                bNeedUpload = TRUE;
            } else {
                up_to_date_count++;
            }
        }

        if (bNeedUpload) {
            if (FtpPutFileA(hFtpSession, local_list[i].file_name, local_list[i].file_name, 
                            FTP_TRANSFER_TYPE_BINARY, 0)) {
                upload_count++;
            } else {
                skip_count++;
            }
        }
    }

    sprintf(status_msg, "Mirror Up Complete. Uploaded: %d, Skipped: %d, Failed: %d", 
            upload_count, up_to_date_count, skip_count);
}

/* メイン管理ループ (引数解析とホールド機能搭載) */
int main(int argc, char* argv[]) {
    HINTERNET hInternet;
    HINTERNET hFtpSession;
    int ch;
    BOOL bRunning;

    /* コマンドライン引数から受け取る変数群 */
    char server[BUFFER_SIZE];
    char user[BUFFER_SIZE];
    char pass[BUFFER_SIZE];
    char remote_folder[MAX_PATH];
    char local_folder[MAX_PATH];

    /* 引数の数が足りない場合、仕様説明（使い方）を表示してプログラムを一時停止（Hold）して終了する */
    if (argc < 6) {
        printf("====================================================================\n");
        printf(" [Usage Error] Missing arguments.\n");
        printf("====================================================================\n");
        printf(" Please execute via Command Line with the following parameters:\n\n");
        printf("   %s <server> <user> <pass> <remote_path> <local_path>\n\n", argv[0]);
        printf(" Example:\n");
        printf("   %s ftp.example.com myusername mypassword /public_html C:\\ftptest\n", argv[0]);
        printf("====================================================================\n");
        printf("\nPress ENTER key to close this window...");
        getchar(); /* 画面をキープ(Hold) */
        return 1;
    }

    /* 引数から各安全バッファへコピー */
    strncpy(server, argv[1], BUFFER_SIZE - 1);
    strncpy(user, argv[2], BUFFER_SIZE - 1);
    strncpy(pass, argv[3], BUFFER_SIZE - 1);
    strncpy(remote_folder, argv[4], MAX_PATH - 1);
    strncpy(local_folder, argv[5], MAX_PATH - 1);

    bRunning = TRUE;

    /* WinINetの初期化 */
    hInternet = InternetOpenA("MirrorFTPClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("Failed to initialize Internet engine.\n");
        printf("Press ENTER key to exit...");
        getchar(); /* 画面をキープ(Hold) */
        return 1;
    }

    /* FTPセッションの開始 */
    hFtpSession = InternetConnectA(hInternet, server, INTERNET_INVALID_PORT_NUMBER, 
                                   user, pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (hFtpSession == NULL) {
        InternetCloseHandle(hInternet);
        printf("FTP Connection Failed. Check server address, username, or password.\n");
        printf("Press ENTER key to exit...");
        getchar(); /* 画面をキープ(Hold) */
        return 1;
    }

    /* PDCursesの初期化 */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    /* メインUIループ */
    while (bRunning) {
        DrawInterface(server, user, remote_folder, local_folder);
        ch = getch();

        switch (ch) {
            case 'u':
            case 'U':
                strcpy(status_msg, "Processing Mirroring Upload... Please wait.");
                DrawInterface(server, user, remote_folder, local_folder);
                PerformMirroringUpload(hFtpSession, remote_folder, local_folder);
                break;

            case 'd':
            case 'D':
                strcpy(status_msg, "Processing Mirroring Download... Please wait.");
                DrawInterface(server, user, remote_folder, local_folder);
                PerformMirroringDownload(hFtpSession, remote_folder, local_folder);
                break;

            case 'q':
            case 'Q':
                bRunning = FALSE;
                break;

            default:
                break;
        }
    }

    /* クリーンアップ */
    endwin();
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    /* 完全にアプリを閉じる前に確認を入れる（Hold） */
    printf("\nSession closed safely. Press ENTER to exit app.");
    getchar();

    return 0;
}