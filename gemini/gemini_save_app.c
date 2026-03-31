#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// curlコマンドの最大長を定義。質問内容やAPIキーで長くなる可能性があるので、余裕を持たせる。
#define MAX_CURL_CMD_LEN 4096

int main(int argc, char *argv[]) {
    // コマンドライン引数が正しく渡されたか確認
    if (argc != 2) {
        fprintf(stderr, "使い方: %s \"あなたの質問内容\"\n", argv[0]);
        fprintf(stderr, "例: %s \"日本の首都はどこですか？\"\n", argv[0]);
        return 1; // エラーコードで終了
    }

    const char *prompt = argv[1]; // コマンドライン引数の2番目が質問内容
    const char *api_key = getenv("GEMINI_API_KEY"); // 環境変数からAPIキーを取得

    // APIキーが設定されているか確認
    if (api_key == NULL) {
        fprintf(stderr, "エラー: 環境変数 GEMINI_API_KEY が設定されていません。\n");
        fprintf(stderr, "APIキーを設定してください (例: export GEMINI_API_KEY=\"YOUR_API_KEY\")\n");
        return 1; // エラーコードで終了
    }

    char curl_cmd[MAX_CURL_CMD_LEN]; // curlコマンドを格納するバッファ

    // snprintfを使ってcurlコマンド文字列を安全に構築
    // -X POST: HTTP POSTメソッドを使用
    // -H "Content-Type: application/json": リクエストボディの形式をJSONに指定
    // -d '{"contents":[{"parts":[{"text":"%s"}]}]}': JSON形式で質問内容を送信
    // "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=%s": Gemini APIのエンドポイントとAPIキー
    // > log.txt 2>&1: 標準出力と標準エラー出力をlog.txtにリダイレクト
    //   これにすることで、curl自体のエラーメッセージもlog.txtに記録されます。
    int len = snprintf(curl_cmd, MAX_CURL_CMD_LEN,
                     "curl -X POST -H \"Content-Type: application/json\" "
                     "-d '{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}]}' "
                     "\"https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=%s\" "
                     "> log.txt 2>&1",
                     prompt, api_key);

    // snprintfがバッファオーバーフローを起こしたか確認
    if (len >= MAX_CURL_CMD_LEN || len < 0) {
        fprintf(stderr, "エラー: curlコマンド文字列が長すぎるか、構築に失敗しました。\n");
        return 1;
    }

    printf("Geminiに質問を送信中... 結果は log.txt に保存されます。\n");

    // system()関数を使って構築したcurlコマンドを実行
    int result = system(curl_cmd);

    // system()関数の戻り値をチェック
    if (result == -1) {
        fprintf(stderr, "エラー: system() 関数の実行に失敗しました。\n");
        return 1;
    } else if (result != 0) {
        fprintf(stderr, "エラー: curl コマンドの実行中に問題が発生しました。log.txt を確認してください。\n");
        return 1;
    }

    printf("Geminiからの応答が log.txt に保存されました。\n");
    printf("ログの内容を確認するには、\"cat log.txt\" または \"jq . log.txt\" を実行してください。\n");

    return 0; // 成功終了
}
