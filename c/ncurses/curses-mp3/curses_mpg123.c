#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mpg123.h>
#include <alsa/asoundlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>

#define PCM_DEVICE "default"
#define MAX_FILES 1024

// グローバル変数
char *files[MAX_FILES];
int file_count = 0;
int current_index = 0;
int playing = 0;
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

// 再生関数（スレッド）
void* play_file(void *arg) {
    const char *filename = (const char*) arg;
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size, done;
    int channels, encoding;
    long rate;
    int err;

    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;

    mpg123_init();
    mh = mpg123_new(NULL, &err);
    mpg123_open(mh, filename);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size);

    snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);
    snd_pcm_prepare(pcm_handle);

    playing = 1;
    while (playing && mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
        snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, buffer, done / (channels * 2));
        if (frames < 0) snd_pcm_prepare(pcm_handle);
    }

    free(buffer);
    snd_pcm_close(pcm_handle);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

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
            playing = 0;
            break;
        }
        else if (ch == KEY_DOWN && current_index < file_count - 1) current_index++;
        else if (ch == KEY_UP && current_index > 0) current_index--;
        else if (ch == 10) { // Enter
            playing = 0; // 前の曲を停止
            sleep(1);    // 前スレッド終了待ち
            pthread_create(&play_thread, NULL, play_file, files[current_index]);
        }
        else if (ch == 's') {
            playing = 0; // 再生停止
        }
    }

    endwin();

    for (int i = 0; i < file_count; i++) free(files[i]);

    return 0;
}
