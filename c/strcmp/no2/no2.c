#include <stdio.h>
#include <string.h>

int main(){
	printf("str1 = hello\\0Abc And hello\\0Abd strcmp\n");
	printf("result = ok.\n");
	char str1[] = "hello\0Abc";
	char str2[] = "hello\0Abd";
	if((strcmp(str1,str2)) == 0){
		printf("ok.\n");
	}
	else{
		printf("no\n");
	}
	return 0;
}

