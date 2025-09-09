#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mpg123.h>
#include <ao/ao.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

#define BITS 8
#define MAX_FILES 1024

// グローバル変数
char *files[MAX_FILES];
int file_count = 0;
int current_index = 0;
volatile int playing = 0;
pthread_t play_thread;

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

// 再生スレッド
void* play_file(void *arg) {
    const char *filename = (const char*) arg;
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size, done;
    int channels, encoding;
    long rate;
    int err;

    ao_device *dev;
    ao_sample_format format;

    mh = mpg123_new(NULL, &err);
    if (!mh) return NULL;

    if (mpg123_open(mh, filename) != MPG123_OK) {
        mpg123_delete(mh);
        return NULL;
    }

    mpg123_getformat(mh, &rate, &channels, &encoding);

    // libao の出力設定
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;

    dev = ao_open_live(ao_default_driver_id(), &format, NULL);
    if (!dev) {
        mpg123_close(mh);
        mpg123_delete(mh);
        return NULL;
    }

    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size);

    playing = 1;
    while (playing && mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
        ao_play(dev, (char*)buffer, done);
    }

    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);

    playing = 0;
    return NULL;
}

// メイン
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

    // 初期化
    ao_initialize();
    mpg123_init();

    // ncurses 初期化
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int ch;
    while (1) {
        clear();
        mvprintw(0, 0, "MP3 Player (q=終了, Enter=再生, ↑↓=移動, s=停止)");
        for (int i = 0; i < file_count; i++) {
            if (i == current_index)
                mvprintw(i + 2, 0, "> %s", files[i]);
            else
                mvprintw(i + 2, 0, "  %s", files[i]);
        }
        refresh();

        ch = getch();
        if (ch == 'q') {
            if (playing) {
                playing = 0;
                pthread_join(play_thread, NULL);
            }
            break;
        }
        else if (ch == KEY_DOWN && current_index < file_count - 1) current_index++;
        else if (ch == KEY_UP && current_index > 0) current_index--;
        else if (ch == 10) { // Enter
            if (playing) {
                playing = 0;
                pthread_join(play_thread, NULL);
            }
            pthread_create(&play_thread, NULL, play_file, files[current_index]);
        }
        else if (ch == 's') {
            if (playing) {
                playing = 0;
                pthread_join(play_thread, NULL);
            }
        }
    }

    endwin();
    mpg123_exit();
    ao_shutdown();

    for (int i = 0; i < file_count; i++) free(files[i]);

    return 0;
}
	

