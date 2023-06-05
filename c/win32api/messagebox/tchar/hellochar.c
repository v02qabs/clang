#include <windows.h>

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR lpCmdline,
	int nCmdShow)
{
	TCHAR str[200] = TEXT("HELLO");
	TCHAR *str2  = TEXT("BOX");

	lstrcat(str, str2);
	MessageBox(NULL, str, str2, MB_OK);
	return 0;
}
