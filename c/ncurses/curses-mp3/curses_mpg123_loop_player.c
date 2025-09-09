/* mpg123_curses_loop.c
 *
 * Build:
 *   sudo apt install -y libmpg123-dev libao-dev libncurses5-dev
 *   gcc mpg123_curses_loop.c -o mpg123_curses_loop -lmpg123 -lao -lncurses -lpthread
 *
 * Run:
 *   ./mpg123_curses_loop /path/to/musicdir
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mpg123.h>
#include <ao/ao.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_FILES 4096
#define BITS_PER_BYTE 8

/* --- グローバル共有状態 --- */
char *files[MAX_FILES];
int file_count = 0;

int selected_index = 0;      /* UI上の選択 */
int current_play_index = -1; /* 現在再生中の曲インデックス (-1 = 無音) */

volatile int playing = 0;    /* 1 = 再生中, 0 = 停止 */
volatile int exit_flag = 0;  /* 1 = 終了要求 */

int loop_enabled = 1;        /* 自動ループ（プレイリストを循環） */

pthread_t player_thread;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t player_cond = PTHREAD_COND_INITIALIZER;

/* --- ユーティリティ --- */
int is_mp3(const char *name) {
    const char *ext = strrchr(name, '.');
    return (ext && (strcasecmp(ext, ".mp3") == 0));
}

/* 指定ディレクトリから mp3 ファイルを読み込む（単純にファイル名を収集） */
void load_files(const char *dir) {
    DIR *dp = opendir(dir);
    struct dirent *ent;
    if (!dp) {
        perror("opendir");
        exit(1);
    }
    while ((ent = readdir(dp)) != NULL) {
        if (is_mp3(ent->d_name)) {
            if (file_count >= MAX_FILES) break;
            size_t len = strlen(dir) + 1 + strlen(ent->d_name) + 1;
            files[file_count] = malloc(len);
            snprintf(files[file_count], len, "%s/%s", dir, ent->d_name);
            file_count++;
        }
    }
    closedir(dp);
}

/* 再生スレッドが呼ぶ実際の再生処理（1曲） */
int play_one_file(int index) {
    if (index < 0 || index >= file_count) return -1;

    const char *filename = files[index];
    mpg123_handle *mh = NULL;
    unsigned char *buffer = NULL;
    size_t buffer_size = 0, done = 0;
    int channels = 0, encoding = 0;
    long rate = 0;
    int err = 0;
    ao_device *dev = NULL;
    ao_sample_format format;

    mh = mpg123_new(NULL, &err);
    if (!mh) goto cleanup;

    if (mpg123_open(mh, filename) != MPG123_OK) goto cleanup;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) goto cleanup;

    /* libao 用フォーマット設定 */
    format.bits = mpg123_encsize(encoding) * BITS_PER_BYTE; /* bytes->bits */
    format.channels = channels;
    format.rate = (int)rate;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;

    dev = ao_open_live(ao_default_driver_id(), &format, NULL);
    if (!dev) goto cleanup;

    buffer_size = mpg123_outblock(mh);
    buffer = malloc(buffer_size);
    if (!buffer) goto cleanup;

    /* 実際のデコードループ */
    while (1) {
        /* 中断要求をチェック */
        pthread_mutex_lock(&state_mutex);
        int local_playing = playing;
        int local_exit = exit_flag;
        pthread_mutex_unlock(&state_mutex);
        if (!local_playing || local_exit) break;

        int rv = mpg123_read(mh, buffer, buffer_size, &done);
        if (rv == MPG123_ERR || rv == MPG123_NEED_MORE) {
            /* エラーか予想外の終了 */
            break;
        }
        if (done > 0) {
            ao_play(dev, (char*)buffer, done);
        }
        if (rv == MPG123_DONE) {
            /* 曲終端 */
            break;
        }
    }

cleanup:
    if (buffer) free(buffer);
    if (dev) ao_close(dev);
    if (mh) {
        mpg123_close(mh);
        mpg123_delete(mh);
    }
    return 0;
}

/* プレイヤースレッド
 * - playing フラグと selected_index/current_play_index を参照して動く
 * - main スレッドから pthread_cond_signal で開始/切替を通知する設計
 */
void *player_main(void *arg) {
    (void)arg;
    while (1) {
        /* 待機: 再生開始要求または終了要求まで待つ */
        pthread_mutex_lock(&state_mutex);
        while (!playing && !exit_flag) {
            pthread_cond_wait(&player_cond, &state_mutex);
        }
        if (exit_flag) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }
        /* 再生開始: 再生インデックスを current_play_index に反映 */
        current_play_index = selected_index;
        pthread_mutex_unlock(&state_mutex);

        /* ループ: 曲が終わったら自動で次へ（loop_enabled に従う） */
        while (1) {
            pthread_mutex_lock(&state_mutex);
            int local_playing = playing;
            int local_exit = exit_flag;
            int local_index = current_play_index;
            pthread_mutex_unlock(&state_mutex);

            if (local_exit || !local_playing) break;

            /* 再生実行 */
            play_one_file(local_index);

            /* 再生後の処理: 自動で次へ進めるか停止か */
            pthread_mutex_lock(&state_mutex);
            if (!playing || exit_flag) {
                pthread_mutex_unlock(&state_mutex);
                break;
            }

            /* 次インデックス決定 */
            int next_index = local_index + 1;
            if (next_index >= file_count) {
                if (loop_enabled) next_index = 0;
                else {
                    /* ループ無効なら停止 */
                    playing = 0;
                    pthread_mutex_unlock(&state_mutex);
                    break;
                }
            }
            /* 更新して UI に反映（selected_index も追従） */
            current_play_index = next_index;
            selected_index = next_index;
            pthread_mutex_unlock(&state_mutex);

            /* 繰り返し：次曲へ */
        }
    }
    return NULL;
}

/* UI 表示更新 */
void draw_ui() {
    clear();
    mvprintw(0, 0, "mpg123 Curses Player  (Enter=再生/切替  n=次 p=前 s=停止 l=ループ切替 q=終了)");
    mvprintw(1, 0, "Loop: %s   Playing: %s", loop_enabled ? "ON " : "OFF", playing ? "YES" : "NO ");
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    int start = 0;
    /* 簡易スクロール：選択位置が画面外ならスクロール */
    if (selected_index >= maxy - 4) start = selected_index - (maxy - 4) + 1;
    for (int i = start; i < file_count && i < start + (maxy - 4); i++) {
        if (i == selected_index && i == current_play_index) {
            mvprintw(i - start + 3, 0, "*> %s", files[i]);
        } else if (i == selected_index) {
            mvprintw(i - start + 3, 0, " > %s", files[i]);
        } else if (i == current_play_index) {
            mvprintw(i - start + 3, 0, " * %s", files[i]);
        } else {
            mvprintw(i - start + 3, 0, "   %s", files[i]);
        }
    }
    refresh();
}

/* メイン */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <music-directory>\n", argv[0]);
        return 1;
    }

    load_files(argv[1]);
    if (file_count == 0) {
        fprintf(stderr, "mp3 ファイルが見つかりません: %s\n", argv[1]);
        return 1;
    }

    /* 初期化 */
    ao_initialize();
    mpg123_init();

    /* プレイヤースレッド起動 */
    pthread_create(&player_thread, NULL, player_main, NULL);

    /* ncurses 初期化 */
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE); /* ブロッキングで getch */

    int ch;
    draw_ui();

    while (1) {
        ch = getch();
        if (ch == 'q') {
            pthread_mutex_lock(&state_mutex);
            exit_flag = 1;
            playing = 0;
            pthread_cond_signal(&player_cond);
            pthread_mutex_unlock(&state_mutex);
            break;
        } else if (ch == KEY_UP) {
            if (selected_index > 0) selected_index--;
        } else if (ch == KEY_DOWN) {
            if (selected_index < file_count - 1) selected_index++;
        } else if (ch == 10 || ch == '\r') { /* Enter: 再生/切替 */
            pthread_mutex_lock(&state_mutex);
            if (playing) {
                /* 再生中なら選択曲に切り替える */
                current_play_index = selected_index;
                /* notify: 再生スレッドは current_play_index を参照して次に再生する */
            } else {
                /* 再生開始 */
                playing = 1;
                current_play_index = selected_index;
                pthread_cond_signal(&player_cond);
            }
            pthread_mutex_unlock(&state_mutex);
        } else if (ch == 's') { /* 停止 */
            pthread_mutex_lock(&state_mutex);
            playing = 0;
            pthread_mutex_unlock(&state_mutex);
        } else if (ch == 'n') { /* 次 */
            pthread_mutex_lock(&state_mutex);
            if (file_count > 0) {
                selected_index = (selected_index + 1) % file_count;
                if (playing) {
                    current_play_index = selected_index;
                }
            }
            pthread_mutex_unlock(&state_mutex);
        } else if (ch == 'p') { /* 前 */
            pthread_mutex_lock(&state_mutex);
            if (file_count > 0) {
                selected_index = (selected_index - 1 + file_count) % file_count;
                if (playing) {
                    current_play_index = selected_index;
                }
            }
            pthread_mutex_unlock(&state_mutex);
        } else if (ch == 'l') { /* ループ切替 */
            pthread_mutex_lock(&state_mutex);
            loop_enabled = !loop_enabled;
            pthread_mutex_unlock(&state_mutex);
        }

        draw_ui();
    }

    /* 終了処理 */
    /* wait player thread */
    pthread_join(player_thread, NULL);

    endwin();

    mpg123_exit();
    ao_shutdown();

    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }
    return 0;
}
