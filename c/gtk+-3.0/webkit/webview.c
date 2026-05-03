#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

// アドレスバーでEnterが押された時のコールバック関数
static void on_address_bar_activate(GtkEntry *entry, WebKitWebView *web_view) {
    const gchar *url = gtk_entry_get_text(entry);
    // 入力されたURLをロードする
    webkit_web_view_load_uri(web_view, url);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // ウィンドウの作成
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 縦並びのボックスを作成（アドレスバーとブラウザを配置）
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Webビューの作成
    GtkWidget *web_view = webkit_web_view_new();

    // アドレスバー（テキスト入力欄）の作成
    GtkWidget *address_bar = gtk_entry_new();
    // Enterキーが押された時にURLを読み込むようにシグナルを設定
    g_signal_connect(address_bar, "activate", G_CALLBACK(on_address_bar_activate), web_view);

    // パーツをボックスに配置
    gtk_box_pack_start(GTK_BOX(vbox), address_bar, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), web_view, TRUE, TRUE, 0);

    // 初期URLのロード
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), "https://www.google.com");

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
