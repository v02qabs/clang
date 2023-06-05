#include <windows.h>


LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wp , LPARAM lp)
{
	switch(message)
	{
			case WM_DESTROY:	
					PostQuitMessage(0);
					return 0;
	}

	return DefWindowProc(hwnd,message,wp,lp);
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR lpCmdLine,
	int nCmdShow)
{
	HWND hwnd;
	WNDCLASS winclass;
	MSG message;

	winclass.style = CS_HREDRAW| CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = winclass.cbWndExtra =0;
	winclass.hInstance = hInstance;
	winclass.hIcon = LoadIcon(NULL,  IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = TEXT("HELLO");


	if(!RegisterClass(&winclass)) return 0;

	hwnd = CreateWindow(
			TEXT("HELLO"), TEXT("HELLO"),
	WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	100,100,500,500,NULL,NULL,hInstance,
	NULL);

	if(hwnd == NULL) return 0;

	while(GetMessage(&message, NULL, 0,0 )) DispatchMessage(&message);
	return message.wParam;
}

