#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <stdio.h>

#pragma comment(lib, "gdiplus.lib")

#define IDC_EDIT     101
#define IDC_SAVE     102
#define IDC_LOAD     103
#define IDC_DATE     104
#define IDC_IMAGEBTN 105
#define IDC_IMAGE    106
#define IDC_IMAGENAME 107

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HBITMAP hImage = NULL;
TCHAR currentImagePath[MAX_PATH] = TEXT("");

ULONG_PTR gdiToken;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    // Init Common Controls
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_DATE_CLASSES};
    InitCommonControlsEx(&icex);

    // Init GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.lpszClassName = TEXT("DiaryLight");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(TEXT("DiaryLight"), TEXT("Diary with Calendar & Image"),
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             650, 550, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiToken);
    return (int)msg.wParam;
}

// ---------------- SAVE / LOAD TEXT ----------------
void SaveEntry(HWND hWnd)
{
    SYSTEMTIME st; 
    DateTime_GetSystemtime(GetDlgItem(hWnd, IDC_DATE), &st);
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
    DateTime_GetSystemtime(GetDlgItem(hWnd, IDC_DATE), &st);
    char fname[MAX_PATH];
    sprintf(fname, "%04d%02d%02d.txt", st.wYear, st.wMonth, st.wDay);

    FILE *f = fopen(fname, "r");
    if (!f)
    {
        MessageBox(hWnd, "No entry found for this date.", "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = (char*)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    SetWindowText(GetDlgItem(hWnd, IDC_EDIT), buf);
    free(buf);
}

// ---------------- IMAGE HANDLING ----------------
void LoadMyImage(HWND hWnd)
{
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = TEXT("Image Files\0*.bmp;*.jpg;*.jpeg;*.png;*.gif\0All Files\0*.*\0");
    ofn.lpstrFile = currentImagePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        // Show file name
        SetWindowText(GetDlgItem(hWnd, IDC_IMAGENAME), currentImagePath);

        // Load image with GDI+
        Gdiplus::Bitmap *bmp = Gdiplus::Bitmap::FromFile(currentImagePath, FALSE);
        if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
        {
            HBITMAP hBmp;
            Gdiplus::Color bg(255,255,255,255);
            bmp->GetHBITMAP(bg, &hBmp);

            // Resize to 200x200
            HDC hdc = GetDC(hWnd);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmResized = CreateCompatibleBitmap(hdc, 200, 200);
            SelectObject(hdcMem, hbmResized);

            HDC hdcSrc = CreateCompatibleDC(hdc);
            SelectObject(hdcSrc, hBmp);

            StretchBlt(hdcMem, 0, 0, 200, 200, hdcSrc, 0, 0,
                       bmp->GetWidth(), bmp->GetHeight(), SRCCOPY);

            DeleteDC(hdcSrc);
            DeleteDC(hdcMem);
            ReleaseDC(hWnd, hdc);

            // Replace old image
            if (hImage) DeleteObject(hImage);
            hImage = hbmResized;

            SendMessage(GetDlgItem(hWnd, IDC_IMAGE), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hImage);

            DeleteObject(hBmp);
        }
        delete bmp;
    }
}

// ---------------- WINDOW PROC ----------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch(msg)
    {
    case WM_CREATE:
        CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,
                       10,10,350,280, hwnd,(HMENU)IDC_EDIT,NULL,NULL);

        CreateWindow(DATETIMEPICK_CLASS, "", 
                     WS_CHILD|WS_VISIBLE|DTS_SHORTDATEFORMAT,
                     10,300,150,25, hwnd,(HMENU)IDC_DATE,NULL,NULL);

        CreateWindow("BUTTON","Save",WS_CHILD|WS_VISIBLE,
                     180,300,80,30, hwnd,(HMENU)IDC_SAVE,NULL,NULL);

        CreateWindow("BUTTON","Load",WS_CHILD|WS_VISIBLE,
                     270,300,80,30, hwnd,(HMENU)IDC_LOAD,NULL,NULL);

        CreateWindow("BUTTON","Load Image",WS_CHILD|WS_VISIBLE,
                     10,340,100,30, hwnd,(HMENU)IDC_IMAGEBTN,NULL,NULL);

        CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|SS_BITMAP,
                     380,10,200,200, hwnd,(HMENU)IDC_IMAGE,NULL,NULL);

        CreateWindow("STATIC","No image",WS_CHILD|WS_VISIBLE,
                     380,220,240,20, hwnd,(HMENU)IDC_IMAGENAME,NULL,NULL);
        break;

    case WM_COMMAND:
        switch(LOWORD(w))
        {
        case IDC_SAVE: SaveEntry(hwnd); break;
        case IDC_LOAD: LoadEntry(hwnd); break;
        case IDC_IMAGEBTN: LoadMyImage(hwnd); break;
        }
        break;

    case WM_DESTROY:
        if (hImage) DeleteObject(hImage);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd,msg,w,l);
}
