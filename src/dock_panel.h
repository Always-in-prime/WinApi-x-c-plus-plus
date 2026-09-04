#ifndef DOCK_PANEL_H_
#define DOCK_PANEL_H_

#include <windows.h>
#include <vector>
#include "button.h"

// Dock panel that manages all UI elements and handles user interaction
class DockPanel {
 public:
  DockPanel();

  // Initialize buttons and system tray icon
  void Initialize();

  // Set window handle for system tray integration
  void SetHwnd(HWND hwnd);

  // Render the dock panel to the window
  void Render(HWND hwnd) const;

  // Handle mouse events
  bool HandleMouseMove(HWND hwnd, int x, int y);
  bool HandleMouseLeave(HWND hwnd);
  bool HandleLeftButtonDown(HWND hwnd, int x, int y);

  // Handle system tray messages
  void HandleTrayMessage(HWND hwnd, LPARAM lParam);
  void HandleCommand(WPARAM wParam, HWND hwnd);

  // Cleanup resources
  void Cleanup();

  // Get notify icon data for shell integration
  NOTIFYICONDATAW* GetNotifyIconData() { return &nid_; }

 private:
  // Drawing helpers
  void DrawCloudShape(Graphics& graphics) const;
  void DrawControlButtons(Graphics& graphics) const;
  void DrawAppButtons(Graphics& graphics) const;
  void DrawLinkButtons(Graphics& graphics) const;

  // Create cloud-shaped window region
  HRGN CreateCloudRegion() const;

  std::vector<AppButton> app_buttons_;
  std::vector<AppButton> link_buttons_;
  std::vector<ControlButton> ctrl_buttons_;
  bool is_mouse_inside_;
  NOTIFYICONDATAW nid_{};
};

#endif  // DOCK_PANEL_H_
