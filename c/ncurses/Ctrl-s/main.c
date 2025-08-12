#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

void c19(){
	printw("typing in 19");
	while(1){
		int c = getch();
		if(c== 'q')
			endwin();
	}
}


int main() {
   int c;
    initscr();
    raw(); // possibly what you intended
    keypad(stdscr, TRUE);
   int x,y,h,d;
    x = 10;
    y = 10;
    h=10;
    d= 10;

    while(1){

	mvprintw(y,x,"A");
	noecho();
	c = getch();
	if(c == KEY_LEFT){
		x++;
	}
	else if(c == KEY_RIGHT){
		x--;
	}
	else if(c == 'q'){
		endwin();
	}
	refresh();
   }
    endwin();
}
