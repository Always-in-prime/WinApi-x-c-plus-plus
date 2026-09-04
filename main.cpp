#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>

#include "dock_panel.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================

static DockPanel* g_dock_panel = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      g_dock_panel = new DockPanel();
      g_dock_panel->Initialize();
      g_dock_panel->SetHwnd(hwnd);
      Shell_NotifyIconW(NIM_ADD, g_dock_panel->GetNotifyIconData());
      break;
    }

    case WM_PAINT: {
      if (g_dock_panel) {
        g_dock_panel->Render(hwnd);
      }
      ValidateRect(hwnd, nullptr);
      break;
    }

    case WM_MOUSEMOVE: {
      if (g_dock_panel) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        g_dock_panel->HandleMouseMove(hwnd, x, y);
      }
      break;
    }

    case WM_MOUSELEAVE: {
      if (g_dock_panel) {
        g_dock_panel->HandleMouseLeave(hwnd);
      }
      break;
    }

    case WM_LBUTTONDOWN: {
      if (g_dock_panel) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        g_dock_panel->HandleLeftButtonDown(hwnd, x, y);
      }
      break;
    }

    case WM_USER + 1: {
      if (g_dock_panel) {
        g_dock_panel->HandleTrayMessage(hwnd, lParam);
      }
      break;
    }

    case WM_COMMAND: {
      if (g_dock_panel) {
        g_dock_panel->HandleCommand(wParam, hwnd);
      }
      break;
    }

    case WM_DESTROY: {
      if (g_dock_panel) {
        g_dock_panel->Cleanup();
        delete g_dock_panel;
        g_dock_panel = nullptr;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  GdiplusStartupInput gdiplus_startup_input;
  ULONG_PTR gdiplus_token;
  GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr);

  WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = L"CustomDockPanelClass";

  RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(
      WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"CustomDockPanelClass",
      L"Dock Panel", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 600, 120, nullptr,
      nullptr, hInstance, nullptr);

  if (!hwnd) {
    GdiplusShutdown(gdiplus_token);
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  GdiplusShutdown(gdiplus_token);
  return static_cast<int>(msg.wParam);
}
