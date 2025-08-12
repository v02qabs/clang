#include <ncurses.h>
#include <string.h>

int main() {
  char str[80];

  initscr();            /* ncursesの初期化 */
  cbreak();             /* バッファリングなしで入力 */
  printw("Enter a line: "); /* プロンプト表示 */
  refresh();            /* 画面更新 */

  getstr(str);         /* 1行取得 */

  printw("\nYou entered: %s\n", str); /* 入力された文字列を表示 */
  refresh();            /* 画面更新 */
  getch();             /* 入力待ち */
  endwin();             /* ncurses終了 */

  return 0;
}
