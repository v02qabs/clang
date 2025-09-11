#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <unistd.h>

#define MAX_FILES 1024

char *files[MAX_FILES];
int file_count = 0;
int current_index = 0;

// MP3判定
int is_mp3(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext && strcasecmp(ext, ".mp3") == 0);
}

// ディレクトリからファイル読み取り
void load_files(const char *dir) {
    DIR *dp = opendir(dir);
    struct dirent *ep;
    if (!dp) {
        perror("ディレクトリが開けません");
        exit(1);
    }
    while ((ep = readdir(dp)) != NULL) {
        if (is_mp3(ep->d_name)) {
            files[file_count] = malloc(strlen(dir) + strlen(ep->d_name) + 2);
            sprintf(files[file_count], "%s/%s", dir, ep->d_name);
            file_count++;
            if (file_count >= MAX_FILES) break;
        }
    }
    closedir(dp);
}

// 1曲再生
void play_once(const char *file) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mpg123 \"%s\"", file);
    endwin();
    system(cmd);
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
}

// 選択曲を無限ループ再生
void play_loop(const char *file) {
    int stop = 0;
    while (!stop) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mpg123 \"%s\"", file);
        endwin();
        system(cmd);
        initscr();
        noecho();
        cbreak();
        keypad(stdscr, TRUE);

        mvprintw(0, 0, "ループ再生中: %s", file);
        mvprintw(1, 0, "qキーで停止");
        refresh();

        timeout(500); // 0.5秒ごとにキー入力確認
        int key = getch();
        if (key == 'q') stop = 1;
    }
    timeout(-1);
}

// 全曲を順番にループ再生
void play_all_loop() {
    int stop = 0;
    while (!stop) {
        for (int i = 0; i < file_count && !stop; i++) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "mpg123 \"%s\"", files[i]);
            endwin();
            system(cmd);
            initscr();
            noecho();
            cbreak();
            keypad(stdscr, TRUE);

            mvprintw(0, 0, "全曲ループ再生中...");
            mvprintw(1, 0, "qキーで停止");
            refresh();

            timeout(500);
            int key = getch();
            if (key == 'q') stop = 1;
        }
    }
    timeout(-1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("使い方: %s <ディレクトリ>\n", argv[0]);
        return 1;
    }

    load_files(argv[1]);
    if (file_count == 0) {
        printf("mp3ファイルが見つかりません\n");
        return 1;
    }

    // ncurses 初期化
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int ch;
    while (1) {
        clear();
        mvprintw(0, 0, "MP3 Player (q=終了, Enter=再生, ↑↓=移動, l=選択曲ループ, a=全曲ループ)");
        for (int i = 0; i < file_count; i++) {
            if (i == current_index)
                mvprintw(i + 2, 0, "> %s", files[i]);
            else
                mvprintw(i + 2, 0, "  %s", files[i]);
        }
        refresh();

        ch = getch();
        if (ch == 'q') {
            break;
        }
        else if (ch == KEY_DOWN && current_index < file_count - 1) current_index++;
        else if (ch == KEY_UP && current_index > 0) current_index--;
        else if (ch == 10) { // Enter
            play_once(files[current_index]);
        }
        else if (ch == 'l') { // 選択曲ループ
            play_loop(files[current_index]);
        }
        else if (ch == 'a') { // 全曲ループ
            play_all_loop();
        }
    }

    endwin();
    for (int i = 0; i < file_count; i++) free(files[i]);
    return 0;
}
