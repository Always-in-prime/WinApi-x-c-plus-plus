#include "dock_panel.h"
#include <windowsx.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <cstring>

// ============================================================================
// DOCK PANEL IMPLEMENTATION
// ============================================================================
DockPanel::DockPanel() : is_mouse_inside_(false) {}

void DockPanel::Initialize() {
    ctrl_buttons_.emplace_back(Rect(710, 25, 740, 55), ControlButton::Type::kMinimize);
    ctrl_buttons_.emplace_back(Rect(750, 25, 780, 55), ControlButton::Type::kClose);

    app_buttons_.emplace_back(Rect(60, 130, 120, 190), L"chrome.exe", L"Chrome", L"chrome.png");
    app_buttons_.emplace_back(Rect(140, 130, 200, 190), L"explorer.exe", L"Explorer", L"explorer.png");
    app_buttons_.emplace_back(Rect(220, 130, 280, 190), L"notepad.exe", L"Notepad", L"notepad.png");
    app_buttons_.emplace_back(Rect(300, 130, 360, 190), L"calc.exe", L"Calculator", L"calc.png");
    app_buttons_.emplace_back(Rect(380, 130, 440, 190), L"mspaint.exe", L"Paint", L"paint.png");

    link_buttons_.emplace_back(Rect(60, 30, 120, 90), L"https://github.com", L"GitHub", L"github.png");
    link_buttons_.emplace_back(Rect(140, 30, 200, 90), L"https://youtube.com", L"YouTube", L"youtube.png");
    link_buttons_.emplace_back(Rect(220, 30, 280, 90), L"https://google.com", L"Google", L"google.png");
    link_buttons_.emplace_back(Rect(300, 30, 360, 90), L"https://stackoverflow.com", L"StackOverflow", L"stackoverflow.png");
    link_buttons_.emplace_back(Rect(380, 30, 440, 90), L"https://reddit.com", L"Reddit", L"reddit.png");

    for (auto& btn : app_buttons_) btn.LoadIcon();
    for (auto& btn : link_buttons_) btn.LoadIcon();

    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = nullptr;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_USER + 1;
    nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid_.szTip, L"Custom Dock Panel");
}

void DockPanel::SetHwnd(HWND hwnd) {
    nid_.hWnd = hwnd;
    Shell_NotifyIconW(NIM_ADD, &nid_);
}

void DockPanel::Render(HWND hwnd) const {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    DrawCloudShape(hdc);

    for (const auto& btn : ctrl_buttons_) btn.Draw(hdc);
    for (const auto& btn : link_buttons_) btn.Draw(hdc);
    for (const auto& btn : app_buttons_) btn.Draw(hdc);

    EndPaint(hwnd, &ps);
}

bool DockPanel::HandleMouseMove(HWND hwnd, int x, int y) {
    bool needs_redraw = false;
    if (!is_mouse_inside_) {
        is_mouse_inside_ = true;
        needs_redraw = true;
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
    }

    auto check_hover = [&](auto& btn) {
        bool was_hovered = btn.IsHovered();
        btn.SetHovered(btn.Contains(x, y));
        if (was_hovered != btn.IsHovered()) needs_redraw = true;
    };

    for (auto& btn : ctrl_buttons_) check_hover(btn);
    for (auto& btn : link_buttons_) check_hover(btn);
    for (auto& btn : app_buttons_) check_hover(btn);

    if (needs_redraw) InvalidateRect(hwnd, nullptr, FALSE);
    return needs_redraw;
}

bool DockPanel::HandleMouseLeave(HWND hwnd) {
    is_mouse_inside_ = false;
    bool needs_redraw = false;

    auto clear_hover = [&](auto& btn) {
        if (btn.IsHovered()) {
            btn.SetHovered(false);
            needs_redraw = true;
        }
    };

    for (auto& btn : ctrl_buttons_) clear_hover(btn);
    for (auto& btn : link_buttons_) clear_hover(btn);
    for (auto& btn : app_buttons_) clear_hover(btn);

    if (needs_redraw) InvalidateRect(hwnd, nullptr, FALSE);
    return needs_redraw;
}

bool DockPanel::HandleLeftButtonDown(HWND hwnd, int x, int y) {
    bool handled = false;

    for (const auto& btn : ctrl_buttons_) {
        if (btn.Contains(x, y)) {
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
            if (btn.Contains(x, y)) {
                ShellExecuteW(nullptr, L"open", btn.GetTarget().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                handled = true;
                break;
            }
        }
    }

    if (!handled) {
        for (const auto& btn : app_buttons_) {
            if (btn.Contains(x, y)) {
                ShellExecuteW(nullptr, L"open", btn.GetTarget().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                handled = true;
                break;
            }
        }
    }

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
void DockPanel::DrawCloudShape(HDC hdc) const {
    HRGN hRgn = CreateCloudRegion();

    // 1. Фон облака
    HBRUSH hBgBrush = CreateSolidBrush(RGB(45, 45, 50));
    FillRgn(hdc, hRgn, hBgBrush);
    DeleteObject(hBgBrush);

    // 2. Обводка облака
    // FrameRgn рисует рамку по границе региона. 
    HBRUSH hFrameBrush = CreateSolidBrush(RGB(200, 200, 200));
    FrameRgn(hdc, hRgn, hFrameBrush, 2, 2);
    DeleteObject(hFrameBrush);

    DeleteObject(hRgn);
}

HRGN DockPanel::CreateCloudRegion() const {
    HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
    HRGN hTemp;

    // Собираем облако из перекрывающихся эллипсов и прямоугольного основания
    // Координаты подобраны для аппроксимации исходного пути GraphicsPath (800x180)
    hTemp = CreateEllipticRgn(20, 60, 150, 160);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    hTemp = CreateEllipticRgn(100, 20, 300, 160);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    hTemp = CreateEllipticRgn(250, 5, 450, 160);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    hTemp = CreateEllipticRgn(400, 10, 600, 160);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    hTemp = CreateEllipticRgn(550, 20, 780, 160);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    // Основание облака
    hTemp = CreateRectRgn(50, 100, 750, 175);
    CombineRgn(hRgn, hRgn, hTemp, RGN_OR);
    DeleteObject(hTemp);

    return hRgn;
}