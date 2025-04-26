#include <stdio.h>
#include <string.h>

int main(){
	char str1[] = "HelloAbc";
	char str2[] = "HelloAdB";

	if((strcmp(str1,str2)) == 0){
		printf("ok.\n");
	}
	else{
		printf("no\n");
	}
	return 0;

}

