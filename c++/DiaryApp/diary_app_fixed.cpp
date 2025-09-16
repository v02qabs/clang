// diary_app_fixed.cpp
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

HWND hEdit;
HWND hImgBtn1, hImgBtn2;
Image* g_img1 = nullptr;
Image* g_img2 = nullptr;
WCHAR imgPath1[MAX_PATH] = L"";
WCHAR imgPath2[MAX_PATH] = L"";

ULONG_PTR gdiplusToken;

void LoadImageFile(HWND hwnd, int which) {
    OPENFILENAMEA ofn = {0};
    char szFile[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Image Files\0*.bmp;*.jpg;*.jpeg\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        WCHAR wPath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, szFile, -1, wPath, MAX_PATH);

        if (which == 1) {
            delete g_img1;
            g_img1 = new Image(wPath);
            lstrcpyW(imgPath1, wPath);
        } else {
            delete g_img2;
            g_img2 = new Image(wPath);
            lstrcpyW(imgPath2, wPath);
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL,
                             10, 10, 400, 200, hwnd, (HMENU)IDC_EDIT_TEXT,
                             ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Save", WS_CHILD | WS_VISIBLE,
                     420, 10, 80, 30, hwnd, (HMENU)IDC_BTN_SAVE,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        CreateWindow("BUTTON", "Load", WS_CHILD | WS_VISIBLE,
                     420, 50, 80, 30, hwnd, (HMENU)IDC_BTN_LOAD,
                     ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hImgBtn1 = CreateWindow("BUTTON", "Image1", WS_CHILD | WS_VISIBLE,
                                420, 100, 80, 30, hwnd, (HMENU)IDC_BTN_IMG1,
                                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hImgBtn2 = CreateWindow("BUTTON", "Image2", WS_CHILD | WS_VISIBLE,
                                420, 140, 80, 30, hwnd, (HMENU)IDC_BTN_IMG2,
                                ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_SAVE: {
            char text[2048];
            GetWindowText(hEdit, text, sizeof(text));
            std::ofstream ofs("diary.txt");
            ofs << text;
            ofs.close();
            MessageBox(hwnd, "Diary saved!", "Info", MB_OK);
            break;
        }
        case IDC_BTN_LOAD: {
            std::ifstream ifs("diary.txt");
            if (ifs) {
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                    std::istreambuf_iterator<char>());
                SetWindowText(hEdit, content.c_str());
            }
            MessageBox(hwnd, "Diary loaded!", "Info", MB_OK);
            break;
        }
        case IDC_BTN_IMG1:
            LoadImageFile(hwnd, 1);
            break;
        case IDC_BTN_IMG2:
            LoadImageFile(hwnd, 2);
            break;
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // White background
        HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        FillRect(hdc, &ps.rcPaint, hBrush);

        Graphics graphics(hdc);

        // Draw image1 at (10, 230) 150x150
        if (g_img1) {
            graphics.DrawImage(g_img1, 10, 230, 150, 150);
        }

        // Draw image2 at (200, 230) 150x150
        if (g_img2) {
            graphics.DrawImage(g_img2, 200, 230, 150, 150);
        }

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        delete g_img1;
        delete g_img2;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DiaryAppClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("DiaryAppClass", "Simple Diary App",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             600, 450, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}