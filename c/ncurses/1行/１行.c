#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char str[80];
  int c;
  initscr();            /* ncursesの初期化 */
  cbreak();             /* バッファリングなしで入力 */
  printw("Enter a line: "); /* プロンプト表示 */
  refresh();            /* 画面更新 */

  getstr(str);         /* 1行取得 */

  printw("\nYou entered: %s\n", str); /* 入力された文字列を表示 */
  refresh();  /* 画面更新 */
  while(true){
	c = getch();
	if(c=47){
		exit(0);
  	}
  }


  /* 入力待ち */

  return 0;
}
