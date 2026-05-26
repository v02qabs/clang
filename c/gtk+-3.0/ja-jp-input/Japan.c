#include <gtk/gtk.h>

// サブウィンドウを開く関数
void open_sub_window() {
    GtkWidget *sub_win;
    GtkWidget *label;

    // サブウィンドウの作成
    sub_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(sub_win), "サブウィンドウ");
    gtk_window_set_default_size(GTK_WINDOW(sub_win), 200, 100);
    gtk_container_set_border_width(GTK_CONTAINER(sub_win), 10);

    // 「あ」と表示するラベルの作成（Pango Markupで文字を少し大きくしています）
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='xx-large'>あ</span>");
    
    // ラベルをサブウィンドウに追加
    gtk_container_add(GTK_CONTAINER(sub_win), label);

    // サブウィンドウとその子コンポーネントを表示
    gtk_widget_show_all(sub_win);
}

// エントリーのテキストが変更されたときに呼ばれるコールバック関数
void on_entry_changed(GtkEditable *editable, gpointer user_data) {
    // エントリーから文字列を取得
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

    // 入力された文字列が "a" であるか判定
    if (g_strcmp0(text, "a") == 0) {
        open_sub_window();
    }
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *entry;

    // GTKの初期化
    gtk_init(&argc, &argv);

    // メインウィンドウの作成
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "メインウィンドウ");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    
    // ウィンドウが閉じられたときにクローズ処理を呼ぶ
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 縦方向のボックス（レイアウト用）を作成
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // テキストエントリーの作成
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "ここに文字を入力してください...");
    
    // "changed" シグナルにコールバック関数を接続
    g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), NULL);
    
    // ボックスにエントリーを配置
    gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);

    // メインウィンドウの表示
    gtk_widget_show_all(window);

    // メインループの開始
    gtk_main();

    return 0;
}
