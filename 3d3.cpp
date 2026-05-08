#include "framework.h"
#include "3d3.h"
#include <windows.h>

HBITMAP currentBitmap = NULL, originalBitmap = NULL;
int bitmapWidth = 0, bitmapHeight = 0;
int histValues[256] = { 0 };

HBITMAP SmartLoadBitmap(LPCWSTR filename) {
    HBITMAP hBmp = NULL;
    wchar_t curDir[MAX_PATH], fullPath[MAX_PATH], projDir[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, curDir);

    hBmp = (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBmp) return hBmp;

    wsprintf(fullPath, L"..\\%s", filename);
    hBmp = (HBITMAP)LoadImage(NULL, fullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBmp) return hBmp;

    wsprintf(fullPath, L"..\\..\\%s", filename);
    hBmp = (HBITMAP)LoadImage(NULL, fullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    return hBmp;
}

void ProcessBrightness(HDC hdc, float k, int c) {
    for (int y = 0; y < bitmapHeight; y++) {
        for (int x = 0; x < bitmapWidth; x++) {
            COLORREF color = GetPixel(hdc, x, y);
            int rgb[3] = { GetRValue(color), GetGValue(color), GetBValue(color) };
            for (int i = 0; i < 3; i++) {
                rgb[i] = (int)(rgb[i] * k + c) % 256;
                if (rgb[i] < 0) rgb[i] += 256;
            }
            SetPixel(hdc, x, y, RGB(rgb[0], rgb[1], rgb[2]));
        }
    }
}

void ContrastStretch(HDC hdc, int n, int c) {
    for (int y = 0; y < bitmapHeight; y++) {
        for (int x = 0; x < bitmapWidth; x++) {
            COLORREF color = GetPixel(hdc, x, y);
            int gray = (int)(0.299 * GetRValue(color) + 0.587 * GetGValue(color) + 0.114 * GetBValue(color));
            int newValue;
            if (gray < n) newValue = 0;
            else if (gray > c) newValue = 255;
            else newValue = 1 + (gray - n) * 253 / (c - n);
            SetPixel(hdc, x, y, RGB(newValue, newValue, newValue));
        }
    }
}

void CalculateHistogram(HDC hdc) {
    memset(histValues, 0, sizeof(histValues));
    for (int y = 0; y < bitmapHeight; y++) {
        for (int x = 0; x < bitmapWidth; x++) {
            COLORREF col = GetPixel(hdc, x, y);
            histValues[(int)(0.299 * GetRValue(col) + 0.587 * GetGValue(col) + 0.114 * GetBValue(col))]++;
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        currentBitmap = SmartLoadBitmap(L"bg.bmp");
        if (currentBitmap) {
            BITMAP bm; GetObject(currentBitmap, sizeof(bm), &bm);
            bitmapWidth = bm.bmWidth; bitmapHeight = bm.bmHeight;
            HDC hdc = GetDC(hwnd);
            HDC hdcSrc = CreateCompatibleDC(hdc), hdcDst = CreateCompatibleDC(hdc);
            originalBitmap = CreateCompatibleBitmap(hdc, bitmapWidth, bitmapHeight);
            SelectObject(hdcSrc, currentBitmap); SelectObject(hdcDst, originalBitmap);
            BitBlt(hdcDst, 0, 0, bitmapWidth, bitmapHeight, hdcSrc, 0, 0, SRCCOPY);
            CalculateHistogram(hdcSrc);
            DeleteDC(hdcSrc); DeleteDC(hdcDst); ReleaseDC(hwnd, hdc);
        }
        else {
            MessageBox(hwnd, L"Файл bg.bmp не найден ни в одной из папок!", L"Ошибка", MB_OK);
        }
    } break;

    case WM_KEYDOWN: {
        if (!currentBitmap) break;
        HDC hdc = GetDC(hwnd);
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, currentBitmap);

        if (wParam == 'R') { 
            HDC hdcOrg = CreateCompatibleDC(hdc);
            SelectObject(hdcOrg, originalBitmap);
            BitBlt(hdcMem, 0, 0, bitmapWidth, bitmapHeight, hdcOrg, 0, 0, SRCCOPY);
            DeleteDC(hdcOrg);
        }
        else if (wParam == '1') ProcessBrightness(hdcMem, 0.5f, 0);
        else if (wParam == '2') ProcessBrightness(hdcMem, 1.5f, 0);
        else if (wParam == '3') ProcessBrightness(hdcMem, 1.0f, 50);
        else if (wParam == '4') ProcessBrightness(hdcMem, 1.0f, -50);
        else if (wParam == '5') ContrastStretch(hdcMem, 50, 200);

        CalculateHistogram(hdcMem);
        DeleteDC(hdcMem); ReleaseDC(hwnd, hdc);
        InvalidateRect(hwnd, NULL, TRUE);
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        if (currentBitmap) {
            HDC memDC = CreateCompatibleDC(hdc);
            SelectObject(memDC, currentBitmap);
            BitBlt(hdc, 10, 10, bitmapWidth, bitmapHeight, memDC, 0, 0, SRCCOPY);

            int maxVal = 0;
            for (int i = 0; i < 256; i++) {
                if (histValues[i] > maxVal) maxVal = histValues[i];
            }
            if (maxVal == 0) maxVal = 1;

            int histHeight = 100;
            int histY = bitmapHeight + 150;

            for (int i = 0; i < 256; i++) {
                int h = (histValues[i] * histHeight) / maxVal;
                if (h > 0) {
                    RECT r = { 10 + i, histY - h, 11 + i, histY };
                    FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
                }
            }

            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 10, bitmapHeight + 15, L"1: *0.5 | 2: *1.5 | 3: +50 | 4: -50 | 5: Контраст | R: Сброс", 58);

            DeleteDC(memDC);
        }
        EndPaint(hwnd, &ps);
    } break;

    case WM_DESTROY:
        if (currentBitmap) DeleteObject(currentBitmap);
        if (originalBitmap) DeleteObject(originalBitmap);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ImageProcClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"ImageProcClass", L"Обработка изображений - Яркость и Контраст",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}