#include <gtk/gtk.h>
#include <string.h>

// ローマ字からひらがなへの簡易マッピング構造体
typedef struct {
    const char *romaji;
    const char *hiragana;
} RomajiMap;

// 簡易的なローマ字・ひらがな変換表（1文字〜3文字に対応）
static RomajiMap romaji_table[] = {
    {"ktsu", "っつ"}, {"kya", "きゃ"}, {"kyu", "きゅ"}, {"kyo", "きょ"},
    {"sha", "しゃ"}, {"shu", "しゅ"}, {"sho", "しょ"}, {"cha", "ちゃ"},
    {"chu", "ちゅ"}, {"cho", "ちょ"}, {"nya", "にゃ"}, {"nyu", "にゅ"},
    {"nyo", "にょ"}, {"hya", "ひゃ"}, {"hyu", "ひゅ"}, {"hyo", "ひょ"},
    {"mya", "みゃ"}, {"myu", "みゅ"}, {"myo", "みょ"}, {"rya", "りゃ"},
    {"ryu", "りゅ"}, {"ryo", "りょ"}, {"gya", "ぎゃ"}, {"gyu", "ぎゅ"},
    {"gyo", "ぎょ"}, {"ja", "じゃ"}, {"ju", "じゅ"}, {"jo", "じょ"},
    {"bya", "びゃ"}, {"byu", "びゅ"}, {"byo", "びょ"}, {"pya", "ぴゃ"},
    {"pyu", "ぴゅ"}, {"pyo", "ぴょ"}, {"tsu", "つ"}, {"shi", "し"},
    {"chi", "ち"}, {"ka", "か"}, {"ki", "き"}, {"ku", "く"}, {"ke", "け"},
    {"ko", "こ"}, {"sa", "さ"}, {"si", "し"}, {"su", "す"}, {"se", "せ"},
    {"so", "そ"}, {"ta", "た"}, {"ti", "ち"}, {"tu", "つ"}, {"te", "て"},
    {"to", "と"}, {"na", "な"}, {"ni", "に"}, {"nu", "ぬ"}, {"ne", "ね"},
    {"no", "の"}, {"ha", "は"}, {"hi", "ひ"}, {"fu", "ふ"}, {"he", "へ"},
    {"ho", "ほ"}, {"ma", "ま"}, {"mi", "み"}, {"mu", "む"}, {"me", "め"},
    {"mo", "も"}, {"ya", "や"}, {"yu", "ゆ"}, {"yo", "よ"}, {"ra", "ら"},
    {"ri", "り"}, {"ru", "る"}, {"re", "れ"}, {"ro", "ろ"}, {"wa", "わ"},
    {"wo", "を"}, {"nn", "ん"}, {"ga", "が"}, {"gi", "ぎ"}, {"gu", "ぐ"},
    {"ge", "げ"}, {"go", "ご"}, {"za", "ざ"}, {"zi", "じ"}, {"zu", "ず"},
    {"ze", "ぜ"}, {"zo", "そ"}, {"da", "だ"}, {"di", "ぢ"}, {"du", "づ"},
    {"de", "で"}, {"do", "ど"}, {"ba", "ば"}, {"bi", "び"}, {"bu", "ぶ"},
    {"be", "べ"}, {"bo", "ぼ"}, {"pa", "ぱ"}, {"pi", "ぴ"}, {"pu", "ぷ"},
    {"pe", "ぺ"}, {"po", "ぽ"}, {"a", "あ"}, {"i", "い"}, {"u", "う"},
    {"e", "え"}, {"o", "お"}, {"n", "ん"}, {NULL, NULL}
};

// ローマ字文字列をひらがなに変換する関数
void convert_romaji_to_hiragana(const char *input, char *output, size_t max_len) {
    output[0] = '\0';
    size_t i = 0;
    size_t input_len = strlen(input);

    while (i < input_len) {
        gboolean matched = FALSE;

        // 促音（っ）の処理: 同じ子音が2つ続いた場合（n以外）
        if (i + 1 < input_len && input[i] == input[i+1] && input[i] != 'n' && 
            ((input[i] >= 'a' && input[i] <= 'z'))) {
            strncat(output, "っ", max_len - strlen(output) - 1);
            i++;
            continue;
        }

        // 変換テーブルからマッチするものを探す（最長一致）
        for (int j = 0; romaji_table[j].romaji != NULL; j++) {
            size_t r_len = strlen(romaji_table[j].romaji);
            if (i + r_len <= input_len && strncmp(&input[i], romaji_table[j].romaji, r_len) == 0) {
                strncat(output, romaji_table[j].hiragana, max_len - strlen(output) - 1);
                i += r_len;
                matched = TRUE;
                break;
            }
        }

        // マッチしない文字はそのまま出力
        if (!matched) {
            char tmp[2] = {input[i], '\0'};
            strncat(output, tmp, max_len - strlen(output) - 1);
            i++;
        }
    }
}

// 左のエントリー（入力）が変更されたときに呼ばれるコールバック
static void on_input_changed(GtkEditable *editable, gpointer user_data) {
    GtkEntry *output_entry = GTK_ENTRY(user_data);
    const char *input_text = gtk_entry_get_text(GTK_ENTRY(editable));
    
    char output_text[4096];
    convert_romaji_to_hiragana(input_text, output_text, sizeof(output_text));
    
    gtk_entry_set_text(output_entry, output_text);
}

// コピーボタンが押されたときに呼ばれるコールバック
static void on_copy_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *output_entry = GTK_ENTRY(user_data);
    const char *text_to_copy = gtk_entry_get_text(output_entry);
    
    // クリップボードを取得してテキストを格納
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, text_to_copy, -1);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // ウィンドウの設定
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ローマ字・ひらがな変換機");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 100);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // レイアウト用ボックス（横並び）
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    // テキストボックス（左：入力用）
    GtkWidget *input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_entry), "romaji...");
    gtk_box_pack_start(GTK_BOX(hbox), input_entry, TRUE, TRUE, 0);

    // テキストボックス（右：出力用）
    GtkWidget *output_entry = gtk_entry_new();

 // 編集不可にする
    gtk_box_pack_start(GTK_BOX(hbox), output_entry, TRUE, TRUE, 0);
    gtk_editable_set_editable(GTK_EDITABLE(output_entry), FALSE);

    // コピーボタン
    GtkWidget *copy_button = gtk_button_new_with_label("コピー");
    gtk_box_pack_start(GTK_BOX(hbox), copy_button, FALSE, FALSE, 0);

    // シグナルの接続
    g_signal_connect(input_entry, "changed", G_CALLBACK(on_input_changed), output_entry);
    g_signal_connect(copy_button, "clicked", G_CALLBACK(on_copy_clicked), output_entry);

    // 画面表示
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
