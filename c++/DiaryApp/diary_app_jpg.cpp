#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <string>
#include <fstream>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define IDC_EDIT_TEXT   101
#define IDC_BTN_SAVE    102
#define IDC_BTN_LOAD    103
#define IDC_BTN_IMG1    104
#define IDC_BTN_IMG2    105
#define IDC_IMG1        106
#define IDC_IMG2        107

char imgPath1[MAX_PATH] = "";
char imgPath2[MAX_PATH] = "";

ULONG_PTR gdiToken;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL SelectImageFile(HWND hwnd, char* outPath);
HBITMAP LoadPicture(LPCWSTR filename, int width, int height);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Start GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("DiaryApp");
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, TEXT("Simple Diary App"),
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             600, 400, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Shutdown GDI+
    GdiplusShutdown(gdiToken);
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit, hImg1, hImg2;

    switch(msg) {
    case WM_CREATE:
        hEdit = CreateWindow("EDIT", "", WS_CHILD|WS_VISIBLE|ES_MULTILINE|WS_BORDER,
                             10, 10, 560, 150, hwnd, (HMENU)IDC_EDIT_TEXT,
                             ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Save", WS_CHILD|WS_VISIBLE,
                     10, 170, 80, 30, hwnd, (HMENU)IDC_BTN_SAVE,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Load", WS_CHILD|WS_VISIBLE,
                     100, 170, 80, 30, hwnd, (HMENU)IDC_BTN_LOAD,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Select Img1", WS_CHILD|WS_VISIBLE,
                     200, 170, 100, 30, hwnd, (HMENU)IDC_BTN_IMG1,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Select Img2", WS_CHILD|WS_VISIBLE,
                     320, 170, 100, 30, hwnd, (HMENU)IDC_BTN_IMG2,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hImg1 = CreateWindow("STATIC", "", WS_CHILD|WS_VISIBLE|SS_BITMAP,
                             10, 210, 250, 150, hwnd, (HMENU)IDC_IMG1,
                             ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hImg2 = CreateWindow("STATIC", "", WS_CHILD|WS_VISIBLE|SS_BITMAP,
                             300, 210, 250, 150, hwnd, (HMENU)IDC_IMG2,
                             ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_BTN_IMG1:
            if (SelectImageFile(hwnd, imgPath1)) {
                HBITMAP hBmp = LoadPicture(std::wstring(imgPath1, imgPath1 + strlen(imgPath1)).c_str(), 250, 150);
                SendMessage(hImg1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
            }
            break;

        case IDC_BTN_IMG2:
            if (SelectImageFile(hwnd, imgPath2)) {
                HBITMAP hBmp = LoadPicture(std::wstring(imgPath2, imgPath2 + strlen(imgPath2)).c_str(), 250, 150);
                SendMessage(hImg2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
            }
            break;

        case IDC_BTN_SAVE: {
            char text[1024];
            GetWindowText(hEdit, text, sizeof(text));

            std::ofstream ofs("diary.txt");
            ofs << text << std::endl;
            ofs << imgPath1 << std::endl;
            ofs << imgPath2 << std::endl;
            ofs.close();
            MessageBox(hwnd, "Saved!", "Diary", MB_OK);
            break;
        }

        case IDC_BTN_LOAD: {
            std::ifstream ifs("diary.txt");
            std::string content, p1, p2;
            if (ifs) {
                std::getline(ifs, content);
                std::getline(ifs, p1);
                std::getline(ifs, p2);
                SetWindowText(hEdit, content.c_str());
                if (!p1.empty()) {
                    strcpy(imgPath1, p1.c_str());
                    HBITMAP hBmp = LoadPicture(std::wstring(p1.begin(), p1.end()).c_str(), 250, 150);
                    SendMessage(hImg1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
                }
                if (!p2.empty()) {
                    strcpy(imgPath2, p2.c_str());
                    HBITMAP hBmp = LoadPicture(std::wstring(p2.begin(), p2.end()).c_str(), 250, 150);
                    SendMessage(hImg2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
                }
            }
            break;
        }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

BOOL SelectImageFile(HWND hwnd, char* outPath) {
    OPENFILENAME ofn = {0};
    char szFile[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.bmp;*.jpg;*.jpeg;*.png\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        strcpy(outPath, szFile);
        return TRUE;
    }
    return FALSE;
}

HBITMAP LoadPicture(LPCWSTR filename, int width, int height) {
    Bitmap bmp(filename);
    HBITMAP hBmp = NULL;
    bmp.GetHBITMAP(Color(255, 255, 255), &hBmp);
    return hBmp;
}
