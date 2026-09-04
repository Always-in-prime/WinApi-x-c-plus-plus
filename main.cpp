#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

struct AppButton {
    RectF bounds;
    std::wstring target;
    std::wstring name;
    bool isHovered;
    std::wstring iconPath;
    Image* iconImage;

    AppButton(RectF b, std::wstring t, std::wstring n, std::wstring path)
        : bounds(b), target(t), name(n), isHovered(false), iconPath(path), iconImage(nullptr) {}
};

struct ControlButton {
    RectF bounds;
    std::wstring type; // "minimize" или "close"
    bool isHovered;

    ControlButton(RectF b, std::wstring t) : bounds(b), type(t), isHovered(false) {}
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

std::vector<AppButton> g_appButtons;
std::vector<ControlButton> g_ctrlButtons;
ULONG_PTR g_gdiplusToken;
bool g_isMouseInside = false;
NOTIFYICONDATAW g_nid = {};

// ============================================================================
// ФУНКЦИИ ОТРИСОВКИ
// ============================================================================

void Render(HWND hwnd) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    HDC hdcScreen = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Graphics graphics(&bitmap);

    // Обязательное сглаживание для идеально гладких краев
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

    // 1. Рисуем форму сглаженного облака (непрерывная кривая)
    PointF pts[] = {
        PointF(60.0f, 85.0f),   // низ лево
        PointF(25.0f, 60.0f),   // левый край
        PointF(60.0f, 35.0f),   // верх лево
        PointF(150.0f, 15.0f),  // верхний бугор 1
        PointF(300.0f, 10.0f),  // верхний центральный бугор
        PointF(450.0f, 15.0f),  // верхний бугор 3
        PointF(540.0f, 35.0f),  // верх право
        PointF(575.0f, 60.0f),  // правый край
        PointF(540.0f, 85.0f),  // низ право
        PointF(450.0f, 105.0f), // нижний бугор 3
        PointF(300.0f, 110.0f), // нижний центральный бугор
        PointF(150.0f, 105.0f)  // нижний бугор 1
    };

    GraphicsPath cloudPath;
    cloudPath.AddClosedCurve(pts, 12);

    // Определяем общую непрозрачность окна (40% = 102, 100% = 255)
    BYTE globalAlpha = g_isMouseInside ? 255 : 102;

    // Фон облака (полупрозрачный темный)
    SolidBrush bgBrush(Color(globalAlpha, 30, 30, 35));
    graphics.FillPath(&bgBrush, &cloudPath);

    // Тонкая граница облака
    Pen borderPen(Color(globalAlpha, 255, 255, 255), 1.5f);
    graphics.DrawPath(&borderPen, &cloudPath);

    // 2. Рисуем кнопки управления (Свернуть и Закрыть)
    for (const auto& btn : g_ctrlButtons) {
        RectF r = btn.bounds;
        Color btnColor = btn.isHovered ? Color(255, 255, 255, 255) : Color(globalAlpha, 180, 180, 180);

        // Подсветка при наведении
        if (btn.isHovered) {
            SolidBrush hoverBrush(Color(60, 255, 255, 255));
            graphics.FillEllipse(&hoverBrush, r);
        }

        Pen iconPen(btnColor, 2.0f);
        if (btn.type == L"minimize") {
            graphics.DrawLine(&iconPen, r.X + 8, r.Y + r.Height / 2, r.X + r.Width - 8, r.Y + r.Height / 2);
        }
        else if (btn.type == L"close") {
            graphics.DrawLine(&iconPen, r.X + 8, r.Y + 8, r.X + r.Width - 8, r.Y + r.Height - 8);
            graphics.DrawLine(&iconPen, r.X + r.Width - 8, r.Y + 8, r.X + 8, r.Y + r.Height - 8);
        }
    }

    // 3. Рисуем кнопки-иконки приложений
    for (auto& btn : g_appButtons) {
        RectF r = btn.bounds;

        // Эффект наведения: мягкий полупрозрачный круг-фон
        if (btn.isHovered) {
            RectF hoverRect = r;
            hoverRect.Inflate(12.0f, 12.0f);
            GraphicsPath hoverPath;
            hoverPath.AddEllipse(hoverRect);
            SolidBrush hoverBrush(Color(80, 255, 255, 255));
            graphics.FillPath(&hoverBrush, &hoverPath);
        }

        // Отрисовка иконки или программной заглушки
        RectF iconRect = r;
        iconRect.Inflate(-8.0f, -8.0f);
        if (btn.isHovered) {
            iconRect.Inflate(3.0f, 3.0f); // Легкое увеличение при наведении
        }

        if (btn.iconImage && btn.iconImage->GetLastStatus() == Ok) {
            graphics.DrawImage(btn.iconImage, iconRect);
        }
        else {
            // Программная заглушка (гарантирует работу приложения без внешних PNG файлов)
            GraphicsPath iconPath;
            iconPath.AddEllipse(iconRect);
            SolidBrush iconBrush(Color(globalAlpha, 100, 150, 255));
            graphics.FillPath(&iconBrush, &iconPath);

            FontFamily fontFamily(L"Segoe UI");
            Font font(&fontFamily, 20.0f, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(globalAlpha, 255, 255, 255));
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);

            std::wstring letter = btn.name.substr(0, 1);
            graphics.DrawString(letter.c_str(), -1, &font, r, &stringFormat, &textBrush);
        }
    }

    // 4. Применяем результат через UpdateLayeredWindow (многослойное окно)
    HBITMAP hBitmap;
    bitmap.GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    POINT ptDst = { rcWindow.left, rcWindow.top };
    SIZE sizeWnd = { width, height };
    POINT ptSrc = { 0, 0 };

    BLENDFUNCTION blend = { AC_SRC_OVER, 0, globalAlpha, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    // Очистка ресурсов отрисовки
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcScreen);
}

// ============================================================================
// ОБРАБОТКА СООБЩЕНИЙ ОКНА
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Инициализация кнопок управления (правый верхний угол)
        g_ctrlButtons.emplace_back(RectF(510.0f, 25.0f, 30.0f, 30.0f), L"minimize");
        g_ctrlButtons.emplace_back(RectF(550.0f, 25.0f, 30.0f, 30.0f), L"close");

        // Инициализация кнопок запуска приложений
        g_appButtons.emplace_back(RectF(60.0f, 30.0f, 60.0f, 60.0f), L"chrome.exe", L"Chrome", L"chrome.png");
        g_appButtons.emplace_back(RectF(140.0f, 30.0f, 60.0f, 60.0f), L"explorer.exe", L"Explorer", L"explorer.png");
        g_appButtons.emplace_back(RectF(220.0f, 30.0f, 60.0f, 60.0f), L"notepad.exe", L"Notepad", L"notepad.png");
        g_appButtons.emplace_back(RectF(300.0f, 30.0f, 60.0f, 60.0f), L"https://github.com", L"GitHub", L"github.png");
        g_appButtons.emplace_back(RectF(380.0f, 30.0f, 60.0f, 60.0f), L"https://youtube.com", L"YouTube", L"youtube.png");

        // Загрузка изображений (если файла нет, GetLastStatus() вернет ошибку, и сработает заглушка)
        for (auto& btn : g_appButtons) {
            btn.iconImage = Image::FromFile(btn.iconPath.c_str());
        }

        // Настройка иконки в системном трее
        g_nid.cbSize = sizeof(NOTIFYICONDATAW);
        g_nid.hWnd = hwnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_USER + 1;
        g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcscpy_s(g_nid.szTip, L"Custom Dock Panel");
        Shell_NotifyIconW(NIM_ADD, &g_nid);
        break;
    }

    case WM_PAINT: {
        Render(hwnd);
        ValidateRect(hwnd, nullptr);
        break;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool needsRedraw = false;

        // Отслеживание входа мыши в окно для изменения прозрачности
        if (!g_isMouseInside) {
            g_isMouseInside = true;
            needsRedraw = true;
        }

        // Проверка наведения на кнопки управления
        for (auto& btn : g_ctrlButtons) {
            bool wasHovered = btn.isHovered;
            btn.isHovered = btn.bounds.Contains((float)x, (float)y);
            if (wasHovered != btn.isHovered) needsRedraw = true;
        }

        // Проверка наведения на кнопки приложений
        for (auto& btn : g_appButtons) {
            bool wasHovered = btn.isHovered;
            btn.isHovered = btn.bounds.Contains((float)x, (float)y);
            if (wasHovered != btn.isHovered) needsRedraw = true;
        }

        if (needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        // Запрос сообщения WM_MOUSELEAVE при уходе курсора
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE: {
        g_isMouseInside = false;
        bool needsRedraw = false;

        for (auto& btn : g_ctrlButtons) {
            if (btn.isHovered) { btn.isHovered = false; needsRedraw = true; }
        }
        for (auto& btn : g_appButtons) {
            if (btn.isHovered) { btn.isHovered = false; needsRedraw = true; }
        }

        if (needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool handled = false;

        // 1. Проверка кнопок управления
        for (const auto& btn : g_ctrlButtons) {
            if (btn.bounds.Contains((float)x, (float)y)) {
                if (btn.type == L"minimize") {
                    ShowWindow(hwnd, SW_HIDE); // Скрываем, но оставляем в трее
                }
                else if (btn.type == L"close") {
                    DestroyWindow(hwnd); // Полное закрытие
                }
                handled = true;
                break;
            }
        }

        // 2. Проверка кнопок приложений
        if (!handled) {
            for (const auto& btn : g_appButtons) {
                if (btn.bounds.Contains((float)x, (float)y)) {
                    ShellExecuteW(nullptr, L"open", btn.target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    handled = true;
                    break;
                }
            }
        }

        // 3. Если клик по пустому месту облака — перетаскивание окна
        if (!handled) {
            PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        break;
    }

    case WM_USER + 1: { // Обработка сообщений от иконки в трее
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);

            // Создаем контекстное меню
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Развернуть панель");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 2, L"Выход");

            // Обязательный трюк для корректного закрытия меню при клике вне его
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessage(hwnd, WM_NULL, 0, 0);

            DestroyMenu(hMenu);
        }
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // Развернуть панель
            ShowWindow(hwnd, SW_SHOW);
        }
        else if (LOWORD(wParam) == 2) { // Выход
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_DESTROY: {
        // Корректное удаление иконки из трея
        Shell_NotifyIconW(NIM_DELETE, &g_nid);

        // Освобождение ресурсов GDI+
        for (auto& btn : g_appButtons) {
            if (btn.iconImage) {
                delete btn.iconImage;
            }
        }
        g_appButtons.clear();
        g_ctrlButtons.clear();

        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// ТОЧКА ВХОДА
// ============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Инициализация GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    // 2. Регистрация класса окна
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"CustomDockPanelClass";

    RegisterClassExW(&wc);

    // 3. Создание многослойного окна без рамки и без отображения на панели задач
    // WS_EX_TOOLWINDOW скрывает окно с панели задач Windows
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CustomDockPanelClass",
        L"Dock Panel",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 120,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        GdiplusShutdown(g_gdiplusToken);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 4. Цикл сообщений
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 5. Завершение работы GDI+
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}