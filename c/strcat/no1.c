#include <stdio.h>
#include <string.h>

int main(){
	char str1[] = "Hello";
	char str2[] = "World";
	char *str3 = strcat(str1, str2);
	printf("%s\n", str3);
	return 0;
}

