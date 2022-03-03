#include <string>
#include <iostream>

#include "clx/salgorithm.h"


using namespace std;

int main(int argc, char *argv[])
{
	string s = "Hello, World";
	cout << "ljust" << clx::ljust_copy(s, 30, '*') << endl;
	return 0;
}

