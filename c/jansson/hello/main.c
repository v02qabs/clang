#include <stdio.h>
#include <jansson.h>

int main(void)
{
    // JSONオブジェクトを入れる変数
    json_error_t error;
    json_t* root;

    // 読み込んだデータを入れる変数
    int num;
    int xx;

    // JSONファイルを読み込む
    root = json_load_file("./sample.json", 0, &error);
    // NULL=読込み失敗
    if ( root == NULL )
    {
        printf("[ERR]json load FAILED\n");
        return 1;
    }

    // "num"という名称のオブジェクトを取得し
    // 値をint型として取得する
    num = json_integer_value(json_object_get(root, "num"));
    printf("num: %d\n", num);

    // "xx"という名称のオブジェクトを取得し
    // 値をint型として取得する
    xx = json_integer_value(json_object_get(root, "xx"));
    printf("xx: %d\n", xx);

    return 0;
}


