#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

// ============================================================================
// BUTTON CLASSES
// ============================================================================

class Button {
public:
    virtual ~Button() = default;
    virtual bool Contains(float x, float y) const = 0;
    virtual void Draw(Graphics& graphics, BYTE globalAlpha) const = 0;
    virtual bool IsHovered() const = 0;
    virtual void SetHovered(bool hovered) = 0;
};

class ControlButton : public Button {
public:
    enum class Type { Minimize, Close };

    ControlButton(RectF bounds, Type type)
        : bounds_(bounds), type_(type), isHovered_(false) {}

    bool Contains(float x, float y) const override {
        return bounds_.Contains(x, y);
    }

    void Draw(Graphics& graphics, BYTE globalAlpha) const override {
        Color btnColor = isHovered_ ? Color(255, 255, 255, 255) : Color(globalAlpha, 180, 180, 180);

        if (isHovered_) {
            SolidBrush hoverBrush(Color(60, 255, 255, 255));
            graphics.FillEllipse(&hoverBrush, bounds_);
        }

        Pen iconPen(btnColor, 2.0f);
        if (type_ == Type::Minimize) {
            graphics.DrawLine(&iconPen, 
                bounds_.X + 8, bounds_.Y + bounds_.Height / 2,
                bounds_.X + bounds_.Width - 8, bounds_.Y + bounds_.Height / 2);
        } else {
            graphics.DrawLine(&iconPen, 
                bounds_.X + 8, bounds_.Y + 8,
                bounds_.X + bounds_.Width - 8, bounds_.Y + bounds_.Height - 8);
            graphics.DrawLine(&iconPen, 
                bounds_.X + bounds_.Width - 8, bounds_.Y + 8,
                bounds_.X + 8, bounds_.Y + bounds_.Height - 8);
        }
    }

    Type GetType() const { return type_; }
    bool IsHovered() const override { return isHovered_; }
    void SetHovered(bool hovered) override { isHovered_ = hovered; }

private:
    RectF bounds_;
    Type type_;
    bool isHovered_;
};

class AppButton : public Button {
public:
    AppButton(RectF bounds, std::wstring target, std::wstring name, std::wstring iconPath)
        : bounds_(bounds), target_(std::move(target)), name_(std::move(name)), 
          iconPath_(std::move(iconPath)), isHovered_(false), iconImage_(nullptr) {}

    ~AppButton() override {
        if (iconImage_) {
            delete iconImage_;
        }
    }

    // Non-copyable due to raw pointer
    AppButton(const AppButton&) = delete;
    AppButton& operator=(const AppButton&) = delete;

    // Movable
    AppButton(AppButton&& other) noexcept
        : bounds_(other.bounds_), target_(std::move(other.target_)), 
          name_(std::move(other.name_)), iconPath_(std::move(other.iconPath_)),
          isHovered_(other.isHovered_), iconImage_(other.iconImage_) {
        other.iconImage_ = nullptr;
    }

    AppButton& operator=(AppButton&& other) noexcept {
        if (this != &other) {
            if (iconImage_) delete iconImage_;
            bounds_ = other.bounds_;
            target_ = std::move(other.target_);
            name_ = std::move(other.name_);
            iconPath_ = std::move(other.iconPath_);
            isHovered_ = other.isHovered_;
            iconImage_ = other.iconImage_;
            other.iconImage_ = nullptr;
        }
        return *this;
    }

    bool Contains(float x, float y) const override {
        return bounds_.Contains(x, y);
    }

    void Draw(Graphics& graphics, BYTE globalAlpha) const override {
        if (isHovered_) {
            RectF hoverRect = bounds_;
            hoverRect.Inflate(12.0f, 12.0f);
            GraphicsPath hoverPath;
            hoverPath.AddEllipse(hoverRect);
            SolidBrush hoverBrush(Color(80, 255, 255, 255));
            graphics.FillPath(&hoverBrush, &hoverPath);
        }

        RectF iconRect = bounds_;
        iconRect.Inflate(-8.0f, -8.0f);
        if (isHovered_) {
            iconRect.Inflate(3.0f, 3.0f);
        }

        if (iconImage_ && iconImage_->GetLastStatus() == Ok) {
            graphics.DrawImage(iconImage_, iconRect);
        } else {
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

            std::wstring letter = name_.substr(0, 1);
            graphics.DrawString(letter.c_str(), -1, &font, iconRect, &stringFormat, &textBrush);
        }
    }

    void LoadIcon() {
        iconImage_ = Image::FromFile(iconPath_.c_str());
    }

    const std::wstring& GetTarget() const { return target_; }
    bool IsHovered() const override { return isHovered_; }
    void SetHovered(bool hovered) override { isHovered_ = hovered; }

private:
    RectF bounds_;
    std::wstring target_;
    std::wstring name_;
    std::wstring iconPath_;
    mutable bool isHovered_;
    Image* iconImage_;
};

// ============================================================================
// DOCK PANEL CLASS
// ============================================================================

class DockPanel {
public:
    DockPanel() : isMouseInside_(false) {}

    void Initialize() {
        // Initialize control buttons (top-right corner)
        ctrlButtons_.emplace_back(RectF(510.0f, 25.0f, 30.0f, 30.0f), ControlButton::Type::Minimize);
        ctrlButtons_.emplace_back(RectF(550.0f, 25.0f, 30.0f, 30.0f), ControlButton::Type::Close);

        // Initialize app buttons
        appButtons_.emplace_back(RectF(60.0f, 30.0f, 60.0f, 60.0f), 
            L"chrome.exe", L"Chrome", L"chrome.png");
        appButtons_.emplace_back(RectF(140.0f, 30.0f, 60.0f, 60.0f), 
            L"explorer.exe", L"Explorer", L"explorer.png");
        appButtons_.emplace_back(RectF(220.0f, 30.0f, 60.0f, 60.0f), 
            L"notepad.exe", L"Notepad", L"notepad.png");
        appButtons_.emplace_back(RectF(300.0f, 30.0f, 60.0f, 60.0f), 
            L"https://github.com", L"GitHub", L"github.png");
        appButtons_.emplace_back(RectF(380.0f, 30.0f, 60.0f, 60.0f), 
            L"https://youtube.com", L"YouTube", L"youtube.png");

        // Load icons
        for (auto& btn : appButtons_) {
            btn.LoadIcon();
        }

        // Setup system tray icon
        nid_.cbSize = sizeof(NOTIFYICONDATAW);
        nid_.hWnd = nullptr; // Will be set later
        nid_.uID = 1;
        nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid_.uCallbackMessage = WM_USER + 1;
        nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcscpy_s(nid_.szTip, L"Custom Dock Panel");
    }

    void SetHwnd(HWND hwnd) {
        nid_.hWnd = hwnd;
    }

    void Render(HWND hwnd) const {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int width = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;

        HDC hdcScreen = GetDC(hwnd);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);

        Bitmap bitmap(width, height, PixelFormat32bppARGB);
        Graphics graphics(&bitmap);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

        DrawCloudShape(graphics);
        DrawControlButtons(graphics);
        DrawAppButtons(graphics);

        HBITMAP hBitmap;
        bitmap.GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

        RECT rcWindow;
        GetWindowRect(hwnd, &rcWindow);
        POINT ptDst = { rcWindow.left, rcWindow.top };
        SIZE sizeWnd = { width, height };
        POINT ptSrc = { 0, 0 };

        BLENDFUNCTION blend = { AC_SRC_OVER, 0, GetGlobalAlpha(), AC_SRC_ALPHA };
        UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcScreen);
    }

    bool HandleMouseMove(HWND hwnd, int x, int y) {
        bool needsRedraw = false;

        if (!isMouseInside_) {
            isMouseInside_ = true;
            needsRedraw = true;
        }

        for (auto& btn : ctrlButtons_) {
            bool wasHovered = btn.IsHovered();
            btn.SetHovered(btn.Contains(static_cast<float>(x), static_cast<float>(y)));
            if (wasHovered != btn.IsHovered()) needsRedraw = true;
        }

        for (auto& btn : appButtons_) {
            bool wasHovered = btn.IsHovered();
            btn.SetHovered(btn.Contains(static_cast<float>(x), static_cast<float>(y)));
            if (wasHovered != btn.IsHovered()) needsRedraw = true;
        }

        if (needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return needsRedraw;
    }

    bool HandleMouseLeave(HWND hwnd) {
        isMouseInside_ = false;
        bool needsRedraw = false;

        for (auto& btn : ctrlButtons_) {
            if (btn.IsHovered()) { btn.SetHovered(false); needsRedraw = true; }
        }
        for (auto& btn : appButtons_) {
            if (btn.IsHovered()) { btn.SetHovered(false); needsRedraw = true; }
        }

        if (needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return needsRedraw;
    }

    bool HandleLeftButtonDown(HWND hwnd, int x, int y) {
        bool handled = false;

        for (const auto& btn : ctrlButtons_) {
            if (btn.Contains(static_cast<float>(x), static_cast<float>(y))) {
                if (btn.GetType() == ControlButton::Type::Minimize) {
                    ShowWindow(hwnd, SW_HIDE);
                } else {
                    DestroyWindow(hwnd);
                }
                handled = true;
                break;
            }
        }

        if (!handled) {
            for (const auto& btn : appButtons_) {
                if (btn.Contains(static_cast<float>(x), static_cast<float>(y))) {
                    ShellExecuteW(nullptr, L"open", btn.GetTarget().c_str(), 
                        nullptr, nullptr, SW_SHOWNORMAL);
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

    void HandleTrayMessage(HWND hwnd, LPARAM lParam) {
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

    void HandleCommand(WPARAM wParam, HWND hwnd) {
        if (LOWORD(wParam) == 1) {
            ShowWindow(hwnd, SW_SHOW);
        } else if (LOWORD(wParam) == 2) {
            DestroyWindow(hwnd);
        }
    }

    void Cleanup() {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        appButtons_.clear();
        ctrlButtons_.clear();
    }

    NOTIFYICONDATAW* GetNotifyIconData() { return &nid_; }

private:
    void DrawCloudShape(Graphics& graphics) const {
        PointF pts[] = {
            PointF(60.0f, 85.0f), PointF(25.0f, 60.0f), PointF(60.0f, 35.0f),
            PointF(150.0f, 15.0f), PointF(300.0f, 10.0f), PointF(450.0f, 15.0f),
            PointF(540.0f, 35.0f), PointF(575.0f, 60.0f), PointF(540.0f, 85.0f),
            PointF(450.0f, 105.0f), PointF(300.0f, 110.0f), PointF(150.0f, 105.0f)
        };

        GraphicsPath cloudPath;
        cloudPath.AddClosedCurve(pts, 12);

        BYTE globalAlpha = GetGlobalAlpha();
        SolidBrush bgBrush(Color(globalAlpha, 30, 30, 35));
        graphics.FillPath(&bgBrush, &cloudPath);

        Pen borderPen(Color(globalAlpha, 255, 255, 255), 1.5f);
        graphics.DrawPath(&borderPen, &cloudPath);
    }

    void DrawControlButtons(Graphics& graphics) const {
        BYTE globalAlpha = GetGlobalAlpha();
        for (const auto& btn : ctrlButtons_) {
            btn.Draw(graphics, globalAlpha);
        }
    }

    void DrawAppButtons(Graphics& graphics) const {
        BYTE globalAlpha = GetGlobalAlpha();
        for (const auto& btn : appButtons_) {
            btn.Draw(graphics, globalAlpha);
        }
    }

    BYTE GetGlobalAlpha() const {
        return isMouseInside_ ? 255 : 102;
    }

private:
    std::vector<AppButton> appButtons_;
    std::vector<ControlButton> ctrlButtons_;
    bool isMouseInside_;
    NOTIFYICONDATAW nid_{};
};

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================

static DockPanel* g_dockPanel = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_dockPanel = new DockPanel();
        g_dockPanel->Initialize();
        g_dockPanel->SetHwnd(hwnd);
        Shell_NotifyIconW(NIM_ADD, g_dockPanel->GetNotifyIconData());
        break;
    }

    case WM_PAINT: {
        if (g_dockPanel) {
            g_dockPanel->Render(hwnd);
        }
        ValidateRect(hwnd, nullptr);
        break;
    }

    case WM_MOUSEMOVE: {
        if (g_dockPanel) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            g_dockPanel->HandleMouseMove(hwnd, x, y);
        }
        break;
    }

    case WM_MOUSELEAVE: {
        if (g_dockPanel) {
            g_dockPanel->HandleMouseLeave(hwnd);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (g_dockPanel) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            g_dockPanel->HandleLeftButtonDown(hwnd, x, y);
        }
        break;
    }

    case WM_USER + 1: {
        if (g_dockPanel) {
            g_dockPanel->HandleTrayMessage(hwnd, lParam);
        }
        break;
    }

    case WM_COMMAND: {
        if (g_dockPanel) {
            g_dockPanel->HandleCommand(wParam, hwnd);
        }
        break;
    }

    case WM_DESTROY: {
        if (g_dockPanel) {
            g_dockPanel->Cleanup();
            delete g_dockPanel;
            g_dockPanel = nullptr;
        }
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// ENTRY POINT
// ============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"CustomDockPanelClass";

    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CustomDockPanelClass",
        L"Dock Panel",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 120,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}