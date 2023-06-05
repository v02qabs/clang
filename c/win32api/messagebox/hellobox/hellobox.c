#include <windows.h>

int WINAPI WinMain(
								HINSTANCE hInstance,
								HINSTANCE hPrevInstance,
								PSTR lpCmdLine,
								int nCmdShow)
		{
				MessageBox(NULL, TEXT("HELLO BOX"), TEXT("LAPPING MESSAGEBOX"),
																MB_OK);	
				return 0;
		}


