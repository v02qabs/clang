#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

// Enterキーが押された時にURLを読み込む関数
static void on_url_entry_activate(GtkEntry *entry, gpointer user_data) {
    WebKitWebView *webview = WEBKIT_WEB_VIEW(user_data);
    const gchar *url = gtk_entry_get_text(entry);
    webkit_web_view_load_uri(webview, url);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // 1. ウィンドウの設定
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_title(GTK_WINDOW(window), "C言語 GTK3 ブラウザ");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 2. レイアウト容器 (垂直ボックス) の作成
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 3. テキスト入力欄 (URLバー) の作成
    GtkWidget *url_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(url_entry), "https://www.google.com");
    // ボックスの上部に配置 (拡大しない, 埋める, パディング)
    gtk_box_pack_start(GTK_BOX(vbox), url_entry, FALSE, FALSE, 0);

    // 4. ウェブビューの作成
    WebKitWebView *webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    // URLバーでEnterを押した時に発火するイベント
    g_signal_connect(url_entry, "activate", G_CALLBACK(on_url_entry_activate), webview);
    
    // ウェブビューをスクロール可能にするコンテナに入れる
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(webview));
    
    // ボックスの下部に配置 (拡大する, 埋める, パディング)
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // 最初のページを読み込む
    webkit_web_view_load_uri(webview, "https://search.yahoo.co.jp");

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
