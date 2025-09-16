#define _WIN32_IE 0x0500   // ensure calendar flags (MCS_SINGLESEL) are available

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")

using namespace Gdiplus;

// fallback for missing flag
#ifndef MCS_SINGLESEL
#define MCS_SINGLESEL 0x0004
#endif

// Control IDs
#define IDC_EDIT_TEXT     101
#define IDC_BTN_LOAD_TEXT 102
#define IDC_BTN_SAVE_TEXT 103
#define IDC_BTN_LOAD_IMG1 104
#define IDC_BTN_LOAD_IMG2 105
#define IDC_STATIC_IMG1   106
#define IDC_STATIC_IMG2   107
#define IDC_MONTHCAL      108

HINSTANCE g_hInst = NULL;
HWND hEditText = NULL, hImg1 = NULL, hImg2 = NULL, hCal = NULL;
std::wstring previewImg1, previewImg2;

// Diary folder helpers
static std::wstring GetDiaryBaseFolder()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    PathRemoveFileSpecW(buf);
    std::wstring base = std::wstring(buf) + L"\\DiaryEntries";
    CreateDirectoryW(base.c_str(), NULL);
    return base;
}

static std::wstring GetFolderForDate(const SYSTEMTIME &st)
{
    wchar_t sub[64];
    swprintf_s(sub, 64, L"\\%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    std::wstring folder = GetDiaryBaseFolder() + sub;
    CreateDirectoryW(folder.c_str(), NULL);
    return folder;
}

// --- Text save/load (UTF-16) ---
static void SaveTextFile(HWND hwEdit, const std::wstring &path)
{
    int len = GetWindowTextLengthW(hwEdit);
    std::wstring text;
    text.resize(len);
    GetWindowTextW(hwEdit, &text[0], len + 1);
    std::wofstream out(path.c_str(), std::ios::binary);
    if (out) out << text;
}

static bool LoadTextFile(HWND hwEdit, const std::wstring &path)
{
    if (!PathFileExistsW(path.c_str())) return false;
    std::wifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    std::wstringstream ss;
    ss << in.rdbuf();
    std::wstring s = ss.str();
    SetWindowTextW(hwEdit, s.c_str());
    return true;
}

// --- Image helpers ---
static void SetStaticBitmap(HWND hStatic, HBITMAP hBmpNew)
{
    HGDIOBJ prev = (HGDIOBJ)SendMessageW(hStatic, STM_GETIMAGE, IMAGE_BITMAP, 0);
    SendMessageW(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpNew);
    if (prev) DeleteObject(prev);
}

// load & scale to 100x100
static bool LoadAndSetImageScaled(HWND hStatic, const std::wstring &path)
{
    if (!PathFileExistsW(path.c_str())) return false;
    Bitmap* source = Bitmap::FromFile(path.c_str());
    if (!source) return false;

    Bitmap scaled(100, 100, PixelFormat32bppARGB);
    Graphics g(&scaled);
    g.Clear(Color(255,255,255));
    g.DrawImage(source, 0, 0, 100, 100);

    HBITMAP hBmp = NULL;
    if (scaled.GetHBITMAP(Color(255,255,255), &hBmp) == Ok) {
        SetStaticBitmap(hStatic, hBmp);
    } else {
        if (hBmp) DeleteObject(hBmp);
        delete source;
        return false;
    }
    delete source;
    return true;
}

// file dialog
static bool BrowseAndPreviewImage(HWND hwndOwner, HWND hStaticTarget, std::wstring &outPreviewPath)
{
    OPENFILENAMEW ofn = {};
    wchar_t filename[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = L"JPEG Files\0*.jpg;*.jpeg\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return false;
    outPreviewPath = filename;
    return LoadAndSetImageScaled(hStaticTarget, outPreviewPath);
}

// auto save image
static void SavePreviewImageTo(const std::wstring &previewPath, const std::wstring &destPath)
{
    if (!previewPath.empty()) CopyFileW(previewPath.c_str(), destPath.c_str(), FALSE);
}

// load text+images for date
static void LoadEntriesForDate(const SYSTEMTIME &st)
{
    std::wstring folder = GetFolderForDate(st);
    LoadTextFile(hEditText, folder + L"\\text.txt");
    if (!LoadAndSetImageScaled(hImg1, folder + L"\\img1.jpg")) {
        SetStaticBitmap(hImg1, NULL);
    } else previewImg1 = folder + L"\\img1.jpg";
    if (!LoadAndSetImageScaled(hImg2, folder + L"\\img2.jpg")) {
        SetStaticBitmap(hImg2, NULL);
    } else previewImg2 = folder + L"\\img2.jpg";
}

// --- Window Proc ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        hEditText = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOVSCROLL,
            10, 10, 480, 150, hwnd, (HMENU)IDC_EDIT_TEXT, g_hInst, NULL);

        hImg1 = CreateWindowW(L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP | WS_BORDER,
            10, 170, 100, 100, hwnd, (HMENU)IDC_STATIC_IMG1, g_hInst, NULL);
        hImg2 = CreateWindowW(L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP | WS_BORDER,
            120, 170, 100, 100, hwnd, (HMENU)IDC_STATIC_IMG2, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"Load Text", WS_CHILD | WS_VISIBLE,
            10, 280, 100, 28, hwnd, (HMENU)IDC_BTN_LOAD_TEXT, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"Save Text", WS_CHILD | WS_VISIBLE,
            120, 280, 100, 28, hwnd, (HMENU)IDC_BTN_SAVE_TEXT, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"Load Img1", WS_CHILD | WS_VISIBLE,
            10, 320, 100, 28, hwnd, (HMENU)IDC_BTN_LOAD_IMG1, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"Load Img2", WS_CHILD | WS_VISIBLE,
            120, 320, 100, 28, hwnd, (HMENU)IDC_BTN_LOAD_IMG2, g_hInst, NULL);

        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES };
        InitCommonControlsEx(&icc);
        hCal = CreateWindowW(L"SysMonthCal32", L"", WS_CHILD | WS_VISIBLE | MCS_SINGLESEL,
            250, 170, 240, 200, hwnd, (HMENU)IDC_MONTHCAL, g_hInst, NULL);

        SYSTEMTIME st;
        GetLocalTime(&st);
        LoadEntriesForDate(st);
        break;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == IDC_BTN_LOAD_TEXT) {
            OPENFILENAMEW ofn = {};
            wchar_t fname[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
            ofn.lpstrFile = fname;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) LoadTextFile(hEditText, fname);
        }
        else if (id == IDC_BTN_SAVE_TEXT) {
            OPENFILENAMEW ofn = {};
            wchar_t fname[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
            ofn.lpstrFile = fname;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            if (GetSaveFileNameW(&ofn)) SaveTextFile(hEditText, fname);
        }
        else if (id == IDC_BTN_LOAD_IMG1) {
            if (BrowseAndPreviewImage(hwnd, hImg1, previewImg1)) {
                SYSTEMTIME st; SendMessageW(hCal, MCM_GETCURSEL, 0, (LPARAM)&st);
                std::wstring folder = GetFolderForDate(st);
                std::wstring outpath = folder + L"\\img1.jpg";
                SavePreviewImageTo(previewImg1, outpath);
            }
        }
        else if (id == IDC_BTN_LOAD_IMG2) {
            if (BrowseAndPreviewImage(hwnd, hImg2, previewImg2)) {
                SYSTEMTIME st; SendMessageW(hCal, MCM_GETCURSEL, 0, (LPARAM)&st);
                std::wstring folder = GetFolderForDate(st);
                std::wstring outpath = folder + L"\\img2.jpg";
                SavePreviewImageTo(previewImg2, outpath);
            }
        }
    }
    break;

    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == IDC_MONTHCAL && pnm->code == MCN_SELCHANGE) {
            SYSTEMTIME st;
            SendMessageW(hCal, MCM_GETCURSEL, 0, (LPARAM)&st);
            LoadEntriesForDate(st);
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --- WinMain ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInst = hInstance;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        MessageBoxW(NULL, L"Gdiplus init failed", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DiaryAppClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Diary App", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInstance, NULL);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
