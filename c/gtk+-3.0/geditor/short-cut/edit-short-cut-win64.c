#include <gtk/gtk.h>
#include <stdio.h>

// グローバル変数
GtkWidget *window;
GtkWidget *text_view;
char *current_filepath = NULL; // 現在開いているファイルのパスを保持

// ウィンドウのタイトルを更新する関数
void update_window_title() {
    if (current_filepath != NULL) {
        char *title = g_strdup_printf("GTK3 Editor - %s", current_filepath);
        gtk_window_set_title(GTK_WINDOW(window), title);
        g_free(title);
    } else {
        gtk_window_set_title(GTK_WINDOW(window), "GTK3 Editor - Untitled");
    }
}

// 「開く」の処理
void open_file(GtkMenuItem *item, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char *content;
        gsize length;

        if (g_file_get_contents(filename, &content, &length, NULL)) {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_set_text(buffer, content, length);
            g_free(content);

            // パスを保存してタイトルを更新
            if (current_filepath) g_free(current_filepath);
            current_filepath = g_strdup(filename);
            update_window_title();
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

// 「名前を付けて保存」の共通処理
gboolean save_as_dialog() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (current_filepath != NULL) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), current_filepath);
    }

    gboolean success = FALSE;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        char *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        if (g_file_set_contents(filename, content, -1, NULL)) {
            if (current_filepath) g_free(current_filepath);
            current_filepath = g_strdup(filename);
            update_window_title();
            success = TRUE;
        }
        
        g_free(content);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    return success;
}

// 「上書き保存」の処理
void save_file(GtkMenuItem *item, gpointer user_data) {
    if (current_filepath == NULL) {
        // まだファイル名がない場合は「名前を付けて保存」を呼び出す
        save_as_dialog();
    } else {
        // すでにファイルパスがある場合は直接上書き
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        char *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        g_file_set_contents(current_filepath, content, -1, NULL);
        g_free(content);
    }
}

// 「名前を付けて保存」メニュー用コールバック
void save_as_file(GtkMenuItem *item, gpointer user_data) {
    save_as_dialog();
}

int main(int argc, char *argv[]) {
    // GTKの初期化
    gtk_init(&argc, &argv);

    // メインウィンドウの設定
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    update_window_title();
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 縦方向のレイアウトコンテナ（メニューバーとテキストエリアを分ける）
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // --- ショートカットキー（アクセラレータ）のグループ設定 ---
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    // --- メニューバーの作成 ---
    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu_item = gtk_menu_item_new_with_label("File");
    GtkWidget *file_menu = gtk_menu_new();

    // メニュー項目の作成
    GtkWidget *open_item = gtk_menu_item_new_with_label("Open");
    GtkWidget *save_item = gtk_menu_item_new_with_label("Save");
    GtkWidget *save_as_item = gtk_menu_item_new_with_label("Save As...");
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");

    // メニュー構造の組み立て
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_as_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new()); // 区切り線
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    // --- ショートカットキーの紐付け ---
    // Open: Ctrl + O
    gtk_widget_add_accelerator(open_item, "activate", accel_group, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    // Save: Ctrl + S
    gtk_widget_add_accelerator(save_item, "activate", accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    // Save As: Ctrl + Shift + S
    gtk_widget_add_accelerator(save_as_item, "activate", accel_group, GDK_KEY_S, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    // Quit: Ctrl + Q
    gtk_widget_add_accelerator(quit_item, "activate", accel_group, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // --- シグナル（イベント）の接続 ---
    g_signal_connect(open_item, "activate", G_CALLBACK(open_file), NULL);
    g_signal_connect(save_item, "activate", G_CALLBACK(save_file), NULL);
    g_signal_connect(save_as_item, "activate", G_CALLBACK(save_as_file), NULL);
    g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);

    // --- テキストエリア（スクロール付き）の作成 ---
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // アプリケーションの起動
    gtk_widget_show_all(window);
    gtk_main();

    // メモリーの解放（終了時）
    if (current_filepath) g_free(current_filepath);

    return 0;
}