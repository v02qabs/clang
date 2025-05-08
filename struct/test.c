#include <stdio.h>

typedef char String [1024];

int main(){
	typedef struct{
		String name;
		int hp;
		int attack;
	}Monster;
	Monster num1 = {"武居", 10, 50};
	const String TEMPLATE = "%s: HP=%3d 攻撃：%2d\n";
	printf(TEMPLATE, num1.name, num1.hp, num1.attack);
	return 0;

}

