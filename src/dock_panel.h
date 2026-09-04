#ifndef DOCK_PANEL_H_
#define DOCK_PANEL_H_

#include <windows.h>
#include <vector>
#include "button.h"

class DockPanel {
public:
    DockPanel();
    void Initialize();
    void SetHwnd(HWND hwnd);
    void Render(HWND hwnd) const;

    bool HandleMouseMove(HWND hwnd, int x, int y);
    bool HandleMouseLeave(HWND hwnd);
    bool HandleLeftButtonDown(HWND hwnd, int x, int y);

    void HandleTrayMessage(HWND hwnd, LPARAM lParam);
    void HandleCommand(WPARAM wParam, HWND hwnd);
    void Cleanup();

    NOTIFYICONDATAW* GetNotifyIconData() { return &nid_; }
    HRGN CreateCloudRegion() const;

private:
    void DrawCloudShape(HDC hdc) const;

    std::vector<AppButton> app_buttons_;
    std::vector<AppButton> link_buttons_;
    std::vector<ControlButton> ctrl_buttons_;
    bool is_mouse_inside_;
    NOTIFYICONDATAW nid_{};
};

#endif  // DOCK_PANEL_H_