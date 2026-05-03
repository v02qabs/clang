#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

static void destroy_window_cb(GtkWidget* widget, gpointer data) {
    gtk_main_quit();
}

int main(int argc, char* argv[]) {
    // GTKの初期化
    gtk_init(&argc, &argv);

    // ウィンドウの作成 (GTK 3)
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);
    gtk_window_set_title(GTK_WINDOW(window), "WebKitGTK 4.1 for SliTaz");

    // WebKitビューの作成
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // 子要素の追加 (ここが修正ポイント)
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));

    // URLのロード
    webkit_web_view_load_uri(web_view, "https://www.google.com");

    // ウィンドウを閉じた時にプログラムを終了させる設定
    g_signal_connect(window, "destroy", G_CALLBACK(destroy_window_cb), NULL);

    // すべてのウィジェットを表示
    gtk_widget_show_all(window);

    // メインループ開始
    gtk_main();

    return 0;
}
