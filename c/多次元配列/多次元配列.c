#include <stdio.h>

int main(){
	int score[2][3];
	score[0][0] = 13;
	score[0][1] = 30;
	score[0][2] = 40;
	score[1][0] = 30;
	score[1][1] = 30;


	for(int i = 0; i< 2; i++){
		printf("%d\n", i+1);
		for(int j = 0; j<3; j++){
			printf("%d : %d\n", j+1, score[i][j]);
		}
	}
	return 0;
}

