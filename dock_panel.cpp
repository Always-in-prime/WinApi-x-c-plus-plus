#include "dock_panel.h"
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>

using namespace Gdiplus;

// ============================================================================
// DOCK PANEL IMPLEMENTATION
// ============================================================================

DockPanel::DockPanel() : is_mouse_inside_(false) {}

void DockPanel::Initialize() {
  // Initialize control buttons (top-right corner)
  ctrl_buttons_.emplace_back(RectF(510.0f, 25.0f, 30.0f, 30.0f),
                             ControlButton::Type::kMinimize);
  ctrl_buttons_.emplace_back(RectF(550.0f, 25.0f, 30.0f, 30.0f),
                             ControlButton::Type::kClose);

  // Initialize app buttons
  app_buttons_.emplace_back(RectF(60.0f, 30.0f, 60.0f, 60.0f), L"chrome.exe",
                            L"Chrome", L"chrome.png");
  app_buttons_.emplace_back(RectF(140.0f, 30.0f, 60.0f, 60.0f),
                            L"explorer.exe", L"Explorer", L"explorer.png");
  app_buttons_.emplace_back(RectF(220.0f, 30.0f, 60.0f, 60.0f), L"notepad.exe",
                            L"Notepad", L"notepad.png");
  app_buttons_.emplace_back(RectF(300.0f, 30.0f, 60.0f, 60.0f),
                            L"https://github.com", L"GitHub", L"github.png");
  app_buttons_.emplace_back(RectF(380.0f, 30.0f, 60.0f, 60.0f),
                            L"https://youtube.com", L"YouTube", L"youtube.png");

  // Load icons
  for (auto& btn : app_buttons_) {
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

void DockPanel::SetHwnd(HWND hwnd) { nid_.hWnd = hwnd; }

void DockPanel::Render(HWND hwnd) const {
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
  POINT ptDst = {rcWindow.left, rcWindow.top};
  SIZE sizeWnd = {width, height};
  POINT ptSrc = {0, 0};

  BLENDFUNCTION blend = {AC_SRC_OVER, 0, GetGlobalAlpha(), AC_SRC_ALPHA};
  UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0,
                      &blend, ULW_ALPHA);

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
  }

  for (auto& btn : ctrl_buttons_) {
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

  TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
  TrackMouseEvent(&tme);
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
      } else {
        DestroyWindow(hwnd);
      }
      handled = true;
      break;
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
  } else if (LOWORD(wParam) == 2) {
    DestroyWindow(hwnd);
  }
}

void DockPanel::Cleanup() {
  Shell_NotifyIconW(NIM_DELETE, &nid_);
  app_buttons_.clear();
  ctrl_buttons_.clear();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================

void DockPanel::DrawCloudShape(Graphics& graphics) const {
  PointF pts[] = {
      PointF(60.0f, 85.0f),  PointF(25.0f, 60.0f), PointF(60.0f, 35.0f),
      PointF(150.0f, 15.0f), PointF(300.0f, 10.0f), PointF(450.0f, 15.0f),
      PointF(540.0f, 35.0f), PointF(575.0f, 60.0f), PointF(540.0f, 85.0f),
      PointF(450.0f, 105.0f), PointF(300.0f, 110.0f), PointF(150.0f, 105.0f)};

  GraphicsPath cloud_path;
  cloud_path.AddClosedCurve(pts, 12);

  BYTE global_alpha = GetGlobalAlpha();
  SolidBrush bg_brush(Color(global_alpha, 30, 30, 35));
  graphics.FillPath(&bg_brush, &cloud_path);

  Pen border_pen(Color(global_alpha, 255, 255, 255), 1.5f);
  graphics.DrawPath(&border_pen, &cloud_path);
}

void DockPanel::DrawControlButtons(Graphics& graphics) const {
  BYTE global_alpha = GetGlobalAlpha();
  for (const auto& btn : ctrl_buttons_) {
    btn.Draw(graphics, global_alpha);
  }
}

void DockPanel::DrawAppButtons(Graphics& graphics) const {
  BYTE global_alpha = GetGlobalAlpha();
  for (const auto& btn : app_buttons_) {
    btn.Draw(graphics, global_alpha);
  }
}

BYTE DockPanel::GetGlobalAlpha() const {
  return is_mouse_inside_ ? 255 : 102;
}
