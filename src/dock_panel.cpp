#include "dock_panel.h"
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <cstring>

using namespace Gdiplus;

// ============================================================================
// DOCK PANEL IMPLEMENTATION
// ============================================================================

DockPanel::DockPanel() : is_mouse_inside_(false) {}

void DockPanel::Initialize() {
    // Initialize control buttons (top-right corner)
    ctrl_buttons_.emplace_back(RectF(710.0f, 25.0f, 30.0f, 30.0f),
        ControlButton::Type::kMinimize);
    ctrl_buttons_.emplace_back(RectF(750.0f, 25.0f, 30.0f, 30.0f),
        ControlButton::Type::kClose);

    // Initialize app buttons (bottom row - 5 programs)
    app_buttons_.emplace_back(RectF(60.0f, 130.0f, 60.0f, 60.0f), L"chrome.exe",
        L"Chrome", L"chrome.png");
    app_buttons_.emplace_back(RectF(140.0f, 130.0f, 60.0f, 60.0f),
        L"explorer.exe", L"Explorer", L"explorer.png");
    app_buttons_.emplace_back(RectF(220.0f, 130.0f, 60.0f, 60.0f), L"notepad.exe",
        L"Notepad", L"notepad.png");
    app_buttons_.emplace_back(RectF(300.0f, 130.0f, 60.0f, 60.0f),
        L"calc.exe", L"Calculator", L"calc.png");
    app_buttons_.emplace_back(RectF(380.0f, 130.0f, 60.0f, 60.0f),
        L"mspaint.exe", L"Paint", L"paint.png");

    // Initialize link buttons (top row - 5 web links)
    link_buttons_.emplace_back(RectF(60.0f, 30.0f, 60.0f, 60.0f),
        L"https://github.com", L"GitHub", L"github.png");
    link_buttons_.emplace_back(RectF(140.0f, 30.0f, 60.0f, 60.0f),
        L"https://youtube.com", L"YouTube", L"youtube.png");
    link_buttons_.emplace_back(RectF(220.0f, 30.0f, 60.0f, 60.0f),
        L"https://google.com", L"Google", L"google.png");
    link_buttons_.emplace_back(RectF(300.0f, 30.0f, 60.0f, 60.0f),
        L"https://stackoverflow.com", L"StackOverflow",
        L"stackoverflow.png");
    link_buttons_.emplace_back(RectF(380.0f, 30.0f, 60.0f, 60.0f),
        L"https://reddit.com", L"Reddit", L"reddit.png");

    // Load icons for all buttons
    for (auto& btn : app_buttons_) {
        btn.LoadIcon();
    }
    for (auto& btn : link_buttons_) {
        btn.LoadIcon();
    }

    // Setup system tray icon
    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = nullptr;  // Will be set later
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_USER + 1;
    nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid_.szTip, L"Custom Dock Panel");
}

void DockPanel::SetHwnd(HWND hwnd) {
    nid_.hWnd = hwnd;
    // Регистрация иконки в трее (раньше она только настраивалась, но не добавлялась)
    Shell_NotifyIconW(NIM_ADD, &nid_);
}

void DockPanel::Render(HWND hwnd) const {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    if (width <= 0 || height <= 0) return;

    HDC hdcScreen = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Создаем 32-битный DIB-раздел для корректной работы с альфа-каналом
    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // Отрицательная высота для сверху-вниз (top-down)
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hBitmap) {
        ReleaseDC(hwnd, hdcScreen);
        DeleteDC(hdcMem);
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Очищаем буфер до полностью прозрачного состояния
    memset(pBits, 0, width * height * 4);

    // Рисуем напрямую в HDC через GDI+ (гарантирует сохранение альфа-канала)
    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

    DrawCloudShape(graphics);
    DrawControlButtons(graphics);
    DrawLinkButtons(graphics);
    DrawAppButtons(graphics);

    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    POINT ptDst = { rcWindow.left, rcWindow.top };
    SIZE sizeWnd = { width, height };
    POINT ptSrc = { 0, 0 };

    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcScreen);
}

bool DockPanel::HandleMouseMove(HWND hwnd, int x, int y) {
    bool needs_redraw = false;

    if (!is_mouse_inside_) {
        is_mouse_inside_ = true;
        needs_redraw = true;
        // Запрашиваем уведомление о выходе мыши только при первом входе
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
    }

    for (auto& btn : ctrl_buttons_) {
        bool was_hovered = btn.IsHovered();
        btn.SetHovered(btn.Contains(static_cast<float>(x), static_cast<float>(y)));
        if (was_hovered != btn.IsHovered()) needs_redraw = true;
    }

    for (auto& btn : link_buttons_) {
        bool was_hovered = btn.IsHovered();
        btn.SetHovered(btn.Contains(static_cast<float>(x), static_cast<float>(y)));
        if (was_hovered != btn.IsHovered()) needs_redraw = true;
    }

    for (auto& btn : app_buttons_) {
        bool was_hovered = btn.IsHovered();
        btn.SetHovered(btn.Contains(static_cast<float>(x), static_cast<float>(y)));
        if (was_hovered != btn.IsHovered()) needs_redraw = true;
    }

    if (needs_redraw) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    return needs_redraw;
}

bool DockPanel::HandleMouseLeave(HWND hwnd) {
    is_mouse_inside_ = false;
    bool needs_redraw = false;

    for (auto& btn : ctrl_buttons_) {
        if (btn.IsHovered()) {
            btn.SetHovered(false);
            needs_redraw = true;
        }
    }
    for (auto& btn : link_buttons_) {
        if (btn.IsHovered()) {
            btn.SetHovered(false);
            needs_redraw = true;
        }
    }
    for (auto& btn : app_buttons_) {
        if (btn.IsHovered()) {
            btn.SetHovered(false);
            needs_redraw = true;
        }
    }

    if (needs_redraw) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    return needs_redraw;
}

bool DockPanel::HandleLeftButtonDown(HWND hwnd, int x, int y) {
    bool handled = false;

    for (const auto& btn : ctrl_buttons_) {
        if (btn.Contains(static_cast<float>(x), static_cast<float>(y))) {
            if (btn.GetType() == ControlButton::Type::kMinimize) {
                ShowWindow(hwnd, SW_HIDE);
            }
            else {
                DestroyWindow(hwnd);
            }
            handled = true;
            break;
        }
    }

    if (!handled) {
        for (const auto& btn : link_buttons_) {
            if (btn.Contains(static_cast<float>(x), static_cast<float>(y))) {
                ShellExecuteW(nullptr, L"open", btn.GetTarget().c_str(), nullptr,
                    nullptr, SW_SHOWNORMAL);
                handled = true;
                break;
            }
        }
    }

    if (!handled) {
        for (const auto& btn : app_buttons_) {
            if (btn.Contains(static_cast<float>(x), static_cast<float>(y))) {
                ShellExecuteW(nullptr, L"open", btn.GetTarget().c_str(), nullptr,
                    nullptr, SW_SHOWNORMAL);
                handled = true;
                break;
            }
        }
    }

    // Если клик не по кнопке, начинаем перетаскивание окна
    if (!handled) {
        PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    return handled;
}

void DockPanel::HandleTrayMessage(HWND hwnd, LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1, L"Развернуть панель");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 2, L"Выход");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
        PostMessage(hwnd, WM_NULL, 0, 0);

        DestroyMenu(hMenu);
    }
}

void DockPanel::HandleCommand(WPARAM wParam, HWND hwnd) {
    if (LOWORD(wParam) == 1) {
        ShowWindow(hwnd, SW_SHOW);
    }
    else if (LOWORD(wParam) == 2) {
        DestroyWindow(hwnd);
    }
}

void DockPanel::Cleanup() {
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    app_buttons_.clear();
    link_buttons_.clear();
    ctrl_buttons_.clear();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================

void DockPanel::DrawCloudShape(Graphics& graphics) const {
    PointF pts[] = {
        PointF(50.0f, 140.0f), PointF(20.0f, 100.0f), PointF(50.0f, 60.0f),
        PointF(120.0f, 20.0f), PointF(250.0f, 10.0f), PointF(400.0f, 5.0f),
        PointF(550.0f, 10.0f), PointF(680.0f, 20.0f), PointF(750.0f, 60.0f),
        PointF(780.0f, 100.0f), PointF(750.0f, 140.0f), PointF(680.0f, 160.0f),
        PointF(550.0f, 170.0f), PointF(400.0f, 175.0f), PointF(250.0f, 170.0f),
        PointF(120.0f, 160.0f) };

    GraphicsPath cloud_path;
    cloud_path.AddClosedCurve(pts, _countof(pts));

    SolidBrush bg_brush(Color(255, 45, 45, 50));
    graphics.FillPath(&bg_brush, &cloud_path);

    Pen border_pen(Color(255, 200, 200, 200), 2.0f);
    graphics.DrawPath(&border_pen, &cloud_path);
}

void DockPanel::DrawControlButtons(Graphics& graphics) const {
    for (const auto& btn : ctrl_buttons_) {
        btn.Draw(graphics, 255);
    }
}

void DockPanel::DrawLinkButtons(Graphics& graphics) const {
    for (const auto& btn : link_buttons_) {
        btn.Draw(graphics, 255);
    }
}

void DockPanel::DrawAppButtons(Graphics& graphics) const {
    for (const auto& btn : app_buttons_) {
        btn.Draw(graphics, 255);
    }
}

HRGN DockPanel::CreateCloudRegion() const {
    PointF pts[] = {
        PointF(50.0f, 140.0f), PointF(20.0f, 100.0f), PointF(50.0f, 60.0f),
        PointF(120.0f, 20.0f), PointF(250.0f, 10.0f), PointF(400.0f, 5.0f),
        PointF(550.0f, 10.0f), PointF(680.0f, 20.0f), PointF(750.0f, 60.0f),
        PointF(780.0f, 100.0f), PointF(750.0f, 140.0f), PointF(680.0f, 160.0f),
        PointF(550.0f, 170.0f), PointF(400.0f, 175.0f), PointF(250.0f, 170.0f),
        PointF(120.0f, 160.0f) };

    GraphicsPath cloud_path;
    cloud_path.AddClosedCurve(pts, _countof(pts));

    Region region(&cloud_path);

    // Создаем временный Graphics контекст (требуется API GDI+)
    HDC hdc = GetDC(nullptr);
    Graphics graphics(hdc);

    // Вызываем GetHRGN с одним аргументом - указателем на Graphics
    // Метод сам вернет HRGN, статус проверяем отдельно
    HRGN hRgn = region.GetHRGN(&graphics);

    ReleaseDC(nullptr, hdc);

    // Проверяем статус последней операции над регионом
    if (region.GetLastStatus() != Ok || hRgn == nullptr) {
        return nullptr;
    }

    return hRgn;
}
