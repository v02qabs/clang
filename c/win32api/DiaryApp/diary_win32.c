#include <windows.h>
#include <stdio.h>

#define IDC_EDIT   101
#define IDC_SAVE   102
#define IDC_LOAD   103

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.lpszClassName = "DiaryLight";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("DiaryLight", "Diary (Light Win32)",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             500, 400, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void SaveEntry(HWND hWnd)
{
    SYSTEMTIME st; 
    GetLocalTime(&st);
    char fname[MAX_PATH];
    sprintf(fname, "%04d%02d%02d.txt", st.wYear, st.wMonth, st.wDay);

    HWND hEdit = GetDlgItem(hWnd, IDC_EDIT);
    int len = GetWindowTextLength(hEdit);
    char *buf = (char*)malloc(len + 1);
    GetWindowText(hEdit, buf, len + 1);

    FILE *f = fopen(fname, "w");
    if (f)
    {
        fputs(buf, f);
        fclose(f);
        MessageBox(hWnd, "Diary entry saved!", "Info", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBox(hWnd, "Failed to save file!", "Error", MB_OK | MB_ICONERROR);
    }
    free(buf);
}

void LoadEntry(HWND hWnd)
{
    SYSTEMTIME st; 
    GetLocalTime(&st);
    char fname[MAX_PATH];
    sprintf(fname, "%04d%02d%02d.txt", st.wYear, st.wMonth, st.wDay);

    FILE *f = fopen(fname, "r");
    if (!f)
    {
        MessageBox(hWnd, "No entry found for today.", "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = (char*)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0'; // null terminate
    fclose(f);

    SetWindowText(GetDlgItem(hWnd, IDC_EDIT), buf);
    free(buf);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch(msg)
    {
    case WM_CREATE:
        CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,
                       10,10,460,280, hwnd,(HMENU)IDC_EDIT,NULL,NULL);

        CreateWindow("BUTTON","Save",WS_CHILD|WS_VISIBLE,
                     10,300,100,30, hwnd,(HMENU)IDC_SAVE,NULL,NULL);

        CreateWindow("BUTTON","Load",WS_CHILD|WS_VISIBLE,
                     120,300,100,30, hwnd,(HMENU)IDC_LOAD,NULL,NULL);
        break;

    case WM_COMMAND:
        switch(LOWORD(w))
        {
        case IDC_SAVE: SaveEntry(hwnd); break;
        case IDC_LOAD: LoadEntry(hwnd); break;
        }
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProc(hwnd,msg,w,l);
}
