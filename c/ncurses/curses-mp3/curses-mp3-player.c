/* mpg123_curses_loop.c - 修正版
 *
 * ビルド方法:
 * sudo apt install -y libmpg123-dev libao-dev libncurses5-dev
 * gcc mpg123_curses_loop.c -o mpg123_curses_loop -lmpg123 -lao -lncurses -lpthread
 *
 * 実行方法:
 * ./mpg123_curses_loop /path/to/musicdir
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
#include <locale.h>

#define MAX_FILES 4096
#define BITS_PER_BYTE 8

/* --- グローバル共有状態 --- */
char *files[MAX_FILES];
int file_count = 0;

int selected_index = 0;      /* UI上の選択 */
int current_play_index = -1; /* 現在再生中の曲インデックス (-1 = 無音) */

volatile int playing = 0;    /* 1 = 再生中, 0 = 停止 (再生ループ全体を制御) */
volatile int paused = 0;     /* 1 = 一時停止中, 0 = 再生中 (デコード処理を制御) */
volatile int exit_flag = 0;  /* 1 = 終了要求 */

int loop_enabled = 1;        /* 自動ループ（プレイリストを循環） */
volatile int repeat_one = 0;  /* 1 = 1曲リピート, 0 = OFF */

pthread_t player_thread;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t player_cond = PTHREAD_COND_INITIALIZER;

/* --- ユーティリティ --- */
int is_mp3(const char *name) {
    const char *ext = strrchr(name, '.');
    return (ext && (strcasecmp(ext, ".mp3") == 0));
}

/* 指定ディレクトリから mp3 ファイルを読み込む */
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

/* 再生処理（1曲分） */
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

    /* libao 出力設定 */
    format.bits = mpg123_encsize(encoding) * BITS_PER_BYTE;
    format.channels = channels;
    format.rate = (int)rate;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;

    dev = ao_open_live(ao_default_driver_id(), &format, NULL);
    if (!dev) goto cleanup;

    buffer_size = mpg123_outblock(mh);
    buffer = malloc(buffer_size);
    if (!buffer) goto cleanup;

    /* デコード再生ループ */
    while (1) {
        pthread_mutex_lock(&state_mutex);
        int local_playing = playing;
        int local_exit = exit_flag;
        int local_paused = paused; // 一時停止フラグを取得
        pthread_mutex_unlock(&state_mutex);

        if (local_exit) break;
        if (!local_playing) break;

        // --- 一時停止処理 ---
        if (local_paused) {
            // 一時停止中はスリープし、デコード処理を行わない
            usleep(50000); 
            continue;
        }
        // --- 再生処理 ---
        int rv = mpg123_read(mh, buffer, buffer_size, &done);
        if (rv == MPG123_ERR || rv == MPG123_NEED_MORE) break;
        if (done > 0) {
            ao_play(dev, (char*)buffer, done);
        }
        if (rv == MPG123_DONE) break;
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

/* プレイヤースレッド */
void *player_main(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&state_mutex);
        while (!playing && !exit_flag) {
            pthread_cond_wait(&player_cond, &state_mutex);
        }
        if (exit_flag) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }
        // playing = 1 でプレイループを開始
        // current_play_index は main の Enter/n/p キーで設定済み
        pthread_mutex_unlock(&state_mutex);

        /* 再生ループ */
        while (1) {
            pthread_mutex_lock(&state_mutex);
            int local_playing = playing;
            int local_exit = exit_flag;
            int local_index = current_play_index;
            int local_repeat_one = repeat_one;
            pthread_mutex_unlock(&state_mutex);

            if (local_exit || !local_playing) break;

            play_one_file(local_index);

            // play_one_file が終了した後の処理
            pthread_mutex_lock(&state_mutex);
            if (!playing || exit_flag) {
                // 再生中に stop されたか、終了フラグが立った
                pthread_mutex_unlock(&state_mutex);
                break;
            }
            
            if (local_repeat_one) {
                // 1曲リピートがONの場合、インデックスは変更せず、再度再生を試みる
            } else {
                // プレイリストの次の曲へ
                int next_index = local_index + 1;
                if (next_index >= file_count) {
                    if (loop_enabled) next_index = 0;
                    else {
                        playing = 0; // プレイリストの終端で停止
                        pthread_mutex_unlock(&state_mutex);
                        break;
                    }
                }
                current_play_index = next_index;
                selected_index = next_index;
            }
            pthread_mutex_unlock(&state_mutex);
        }
    }
    return NULL;
}

/* UI */
void draw_ui() {
    clear();
    mvprintw(0, 0, "mpg123 Curses Player");
    mvprintw(1, 0, "Controls: Enter=Play/Switch  SPACE=Pause/Resume  n=Next  p=Prev  s=Stop  l=PlaylistLoop  r=RepeatOne  q=Quit");
    
    char status_str[32];
    if (playing) {
        snprintf(status_str, sizeof(status_str), "%s", paused ? "PAUSED" : "PLAYING");
    } else {
        snprintf(status_str, sizeof(status_str), "STOPPED");
    }
    
    mvprintw(2, 0, "Status: %s | Playlist Loop: %s | Repeat One: %s", 
             status_str, 
             loop_enabled ? "ON " : "OFF",
             repeat_one ? "ON " : "OFF");

    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    int start = 0;
    
    // UIの表示開始行を調整 (コントロール行が3行になったため)
    if (selected_index >= maxy - 5) start = selected_index - (maxy - 5) + 1;
    
    for (int i = start; i < file_count && i < start + (maxy - 5); i++) {
        // 現在の行インデックス
        int row = i - start + 4; 
        
        // ファイル名表示のオフセット (相対パスの最初のディレクトリ部分を削除)
        const char *display_name = files[i];
        const char *last_slash = strrchr(files[i], '/');
        if (last_slash) {
            display_name = last_slash + 1;
        }

        // 選択インデックスと再生インデックスの比較
        int is_selected = (i == selected_index);
        int is_playing = (i == current_play_index && playing && !paused);
        int is_paused = (i == current_play_index && playing && paused);

        if (is_selected && is_playing) {
            attron(A_REVERSE | A_BOLD);
            mvprintw(row, 0, "*> %s", display_name);
            attroff(A_REVERSE | A_BOLD);
        } else if (is_selected && is_paused) {
            attron(A_REVERSE);
            mvprintw(row, 0, "=p %s", display_name); // p: paused
            attroff(A_REVERSE);
        } else if (is_selected) {
            attron(A_REVERSE);
            mvprintw(row, 0, " > %s", display_name);
            attroff(A_REVERSE);
        } else if (is_playing) {
            attron(A_BOLD);
            mvprintw(row, 0, " * %s", display_name);
            attroff(A_BOLD);
        } else if (is_paused) {
            mvprintw(row, 0, " = %s", display_name);
        } else {
            mvprintw(row, 0, "   %s", display_name);
        }
    }
    
    // カーソルを非表示にする
    move(maxy - 1, maxx - 1); 
    
    refresh();
}

/* メイン */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <music-directory>\n", argv[0]);
        return 1;
    }
    // ワイド文字のサポートを有効にする
    setlocale(LC_ALL, ""); 
    
    load_files(argv[1]);
    if (file_count == 0) {
        fprintf(stderr, "mp3 ファイルが見つかりません: %s\n", argv[1]);
        return 1;
    }

    ao_initialize();
    mpg123_init();

    pthread_create(&player_thread, NULL, player_main, NULL);

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0); // カーソルを非表示

    int ch;
    draw_ui();

    while (1) {
        ch = getch();
        
        pthread_mutex_lock(&state_mutex);
        
        if (ch == 'q') {
            exit_flag = 1;
            playing = 0;
            paused = 0;
            pthread_cond_signal(&player_cond);
            pthread_mutex_unlock(&state_mutex);
            break;
        } else if (ch == KEY_UP) {
            if (selected_index > 0) selected_index--;
        } else if (ch == KEY_DOWN) {
            if (selected_index < file_count - 1) selected_index++;
        } else if (ch == 10 || ch == '\r') { /* Enter: 再生開始または曲切替 */
            // 再生中の曲が選択されていれば何もしない (すぐに再生を中断すると音が途切れるため)
            if (playing && current_play_index == selected_index) {
                paused = 0; // 念のため一時停止解除
                pthread_mutex_unlock(&state_mutex);
                continue;
            }
            
            playing = 0; // 現在の再生を停止させる
            paused = 0;  // 一時停止解除
            pthread_mutex_unlock(&state_mutex);
            usleep(50000); // 停止処理を待機 (デコードループが終了するのを少し待つ)
            
            pthread_mutex_lock(&state_mutex);
            playing = 1;
            current_play_index = selected_index;
            pthread_cond_signal(&player_cond);
        } else if (ch == ' ') { /* Space: 一時停止 / 続きを再生 */
            if (playing) {
                paused = !paused;
            } else {
                // 停止中にスペースを押した場合、再生開始
                playing = 1;
                paused = 0;
                current_play_index = selected_index;
                pthread_cond_signal(&player_cond);
            }
        } else if (ch == 's') { /* 停止 */
            playing = 0;
            paused = 0;
            current_play_index = -1; // 再生インデックスをリセット
        } else if (ch == 'n') { /* 次 */
            if (file_count > 0) {
                selected_index = (selected_index + 1) % file_count;
                current_play_index = selected_index;
                playing = 0; paused = 0; // 停止させてから再開
            }
            pthread_mutex_unlock(&state_mutex);
            usleep(50000);
            pthread_mutex_lock(&state_mutex);
            playing = 1;
            pthread_cond_signal(&player_cond);
        } else if (ch == 'p') { /* 前 */
            if (file_count > 0) {
                selected_index = (selected_index - 1 + file_count) % file_count;
                current_play_index = selected_index;
                playing = 0; paused = 0; // 停止させてから再開
            }
            pthread_mutex_unlock(&state_mutex);
            usleep(50000);
            pthread_mutex_lock(&state_mutex);
            playing = 1;
            pthread_cond_signal(&player_cond);
        } else if (ch == 'l') { /* プレイリストループ切替 */
            loop_enabled = !loop_enabled;
        } else if (ch == 'r') { /* 1曲リピート切替 */
            repeat_one = !repeat_one;
        }
        
        pthread_mutex_unlock(&state_mutex);
        draw_ui();
    }

    // 終了処理
    pthread_join(player_thread, NULL);
    endwin();

    mpg123_exit();
    ao_shutdown();

    for (int i = 0; i < file_count; i++) free(files[i]);
    return 0;
}

