#define UNICODE
#define _UNICODE

#include <windows.h>
#include <fstream>
#include <sstream>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <filesystem>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Gdiplus;
namespace fs = std::filesystem;

#define IDC_EDIT     101
#define IDC_SAVE     102
#define IDC_LOAD     103
#define IDC_ADDIMG   104
#define IDC_CALENDAR 105

#define ENTRY_FOLDER L"DiaryEntries"

HINSTANCE g_hInst;
HWND hEdit, hCalendar;
ULONG_PTR gdiToken;
std::vector<HWND> hImageCtrls;

// Utility: 日付プレフィックス生成
std::wstring MakeDatePrefix() {
    SYSTEMTIME st;
    MonthCal_GetCurSel(hCalendar, &st);
    wchar_t buf[32];
    wsprintfW(buf, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
    return buf;
}

// --- テキスト保存 ---
void SaveTextEntry() {
    std::wstring prefix = MakeDatePrefix();
    wchar_t filename[MAX_PATH];
    wsprintfW(filename, L"%s\\%s.txt", ENTRY_FOLDER, prefix.c_str());

    int len = GetWindowTextLengthW(hEdit);
    std::wstring buf(len, L'\0');
    GetWindowTextW(hEdit, &buf[0], len + 1);

    CreateDirectoryW(ENTRY_FOLDER, NULL);

    // Convert to UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, buf.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8Text(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, buf.c_str(), -1, &utf8Text[0], u8len, NULL, NULL);

    std::ofstream file(filename, std::ios::binary);
    file.write(utf8Text.c_str(), utf8Text.size());

    MessageBoxW(NULL, L"Diary entry saved!", L"Info", MB_OK | MB_ICONINFORMATION);
}


// --- テキスト読込 ---
#include <fstream>
#include <sstream>

void LoadTextEntry() {
    std::wstring prefix = MakeDatePrefix();
    wchar_t filename[MAX_PATH];
    wsprintfW(filename, L"%s\\%s.txt", ENTRY_FOLDER, prefix.c_str());

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        SetWindowTextW(hEdit, L"");
    } else {
        // Read entire file into std::string (UTF-8)
        std::stringstream ss;
        ss << file.rdbuf();
        std::string utf8Text = ss.str();

        // Convert UTF-8 string to std::wstring
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, NULL, 0);
        std::wstring wbuf(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, &wbuf[0], wlen);

        SetWindowTextW(hEdit, wbuf.c_str());
    }

    // --- Reload images as before ---
    for (HWND h : hImageCtrls) DestroyWindow(h);
    hImageCtrls.clear();

    int idx = 1;
    while (true) {
        wchar_t imgfile[MAX_PATH];
        wsprintfW(imgfile, L"%s\\%s_img%d.jpg", ENTRY_FOLDER, prefix.c_str(), idx);
        if (!PathFileExistsW(imgfile)) break;

        Bitmap bmp(imgfile);
        if (bmp.GetLastStatus() == Ok) {
            Bitmap resized(150, 150, bmp.GetPixelFormat());
            Graphics g(&resized);
            g.DrawImage(&bmp, 0, 0, 150, 150);

            HBITMAP hbmp;
            Color bg(255, 255, 255, 255);
            resized.GetHBITMAP(bg, &hbmp);

            HWND hImg = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP,
                                     400, 50 + (idx - 1) * 160, 150, 150,
                                     GetParent(hEdit), NULL, g_hInst, NULL);
            SendMessage(hImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
            hImageCtrls.push_back(hImg);
        }
        idx++;
    }
}

// --- 画像表示 ---
void ShowImage(HWND hWnd, const std::wstring& path, int x, int y) {
    Bitmap bmp(path.c_str());
    if (bmp.GetLastStatus() != Ok) return;

    Bitmap resized(150, 150, bmp.GetPixelFormat());
    Graphics g(&resized);
    g.DrawImage(&bmp, 0, 0, 150, 150);

    HBITMAP hbmp;
    Color bg(255, 255, 255, 255);
    resized.GetHBITMAP(bg, &hbmp);

    HWND hImg = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP,
                             x, y, 150, 150, hWnd, NULL, g_hInst, NULL);
    SendMessage(hImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
    hImageCtrls.push_back(hImg);
}

// --- 画像追加 ---
void AddImage(HWND hWnd) {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Image Files\0*.jpg;*.jpeg;*.png;*.bmp\0All Files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring prefix = MakeDatePrefix();
        int idx = 1;
        wchar_t savePath[MAX_PATH];
        while (true) {
            wsprintfW(savePath, L"%s\\%s_img%d.jpg", ENTRY_FOLDER, prefix.c_str(), idx);
            if (!PathFileExistsW(savePath)) break;
            idx++;
        }

        CreateDirectoryW(ENTRY_FOLDER, NULL);
        CopyFileW(file, savePath, FALSE);

        int x = 400, y = 50 + (idx - 1) * 160;
        ShowImage(hWnd, savePath, x, y);
    }
}

// --- WndProc ---
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL,
                               10, 10, 350, 250, hWnd, (HMENU)IDC_EDIT, g_hInst, NULL);

        hCalendar = CreateWindowExW(0, MONTHCAL_CLASS, L"", WS_CHILD | WS_VISIBLE | MCS_DAYSTATE,
                                   10, 270, 250, 200, hWnd, (HMENU)IDC_CALENDAR, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"Save Text", WS_CHILD | WS_VISIBLE,
                     280, 270, 100, 30, hWnd, (HMENU)IDC_SAVE, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"Load Text & Images", WS_CHILD | WS_VISIBLE,
                     280, 310, 100, 30, hWnd, (HMENU)IDC_LOAD, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"Add Image", WS_CHILD | WS_VISIBLE,
                     280, 350, 100, 30, hWnd, (HMENU)IDC_ADDIMG, g_hInst, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SAVE: SaveTextEntry(); break;
        case IDC_LOAD: LoadTextEntry(); break;
        case IDC_ADDIMG: AddImage(hWnd); break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// --- WinMain ---
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInst;

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_DATE_CLASSES | ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DiaryAppAnyImg";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowW(L"DiaryAppAnyImg", L"Diary App (Text + Any Images)",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             700, 600, NULL, NULL, hInst, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(gdiToken);
    return (int)msg.wParam;
}
