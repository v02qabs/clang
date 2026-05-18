#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // ロケールの設定（日本語環境に必須）
    if (setlocale(LC_ALL, "") == NULL) return 1;
    if (!XSupportsLocale()) return 1;

    Display *dpy = XOpenDisplay(NULL);
    int scr = DefaultScreen(dpy);
    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 10, 10, 400, 100, 1,
                                     BlackPixel(dpy, scr), WhitePixel(dpy, scr));

    // XIM (日本語入力) の初期化
    XIM im = XOpenIM(dpy, NULL, NULL, NULL);
    XIC ic = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                       XNClientWindow, win, NULL);

    // Xftフォント (Takaoフォント) のロード
    XftFont *font = XftFontOpenName(dpy, scr, "TakaoGothic-16");
    XftDraw *draw = XftDrawCreate(dpy, win, DefaultVisual(dpy, scr), DefaultColormap(dpy, scr));
    XftColor color;
    XftColorAllocName(dpy, DefaultVisual(dpy, scr), DefaultColormap(dpy, scr), "black", &color);

    XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XMapWindow(dpy, win);

    char buf[128] = "入力: ";
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        if (XFilterEvent(&ev, win)) continue; // XIMのイベントをフィルタリング

        if (ev.type == Expose) {
            XftDrawStringUtf8(draw, &color, font, 20, 50, (FcChar8*)buf, strlen(buf));
        } else if (ev.type == KeyPress) {
            Status stat;
            KeySym keysym;
            char input[32];
            // XIM経由で入力を取得
            int len = Xutf8LookupString(ic, &ev.xkey, input, sizeof(input) - 1, &keysym, &stat);
            if (len > 0) {
                input[len] = '\0';
                strcat(buf, input); // 入力文字列を連結
                XClearWindow(dpy, win);
                XftDrawStringUtf8(draw, &color, font, 20, 50, (FcChar8*)buf, strlen(buf));
            }
        }
    }
    return 0;
}