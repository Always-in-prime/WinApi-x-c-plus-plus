#ifndef DOCK_BUTTON_H_
#define DOCK_BUTTON_H_

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <memory>

using namespace Gdiplus;

// Abstract base class for all buttons
class Button {
 public:
  virtual ~Button() = default;
  virtual bool Contains(float x, float y) const = 0;
  virtual void Draw(Graphics& graphics, BYTE global_alpha) const = 0;
  virtual bool IsHovered() const = 0;
  virtual void SetHovered(bool hovered) = 0;
};

// Control button (minimize/close)
class ControlButton : public Button {
 public:
  enum class Type { kMinimize, kClose };

  explicit ControlButton(RectF bounds, Type type);

  bool Contains(float x, float y) const override;
  void Draw(Graphics& graphics, BYTE global_alpha) const override;
  Type GetType() const { return type_; }
  bool IsHovered() const override { return is_hovered_; }
  void SetHovered(bool hovered) override { is_hovered_ = hovered; }

 private:
  RectF bounds_;
  Type type_;
  bool is_hovered_;
};

// Application button with icon support
class AppButton : public Button {
 public:
  AppButton(RectF bounds, std::wstring target, std::wstring name,
            std::wstring icon_path);
  ~AppButton() override;

  // Non-copyable due to raw pointer
  AppButton(const AppButton&) = delete;
  AppButton& operator=(const AppButton&) = delete;

  // Movable
  AppButton(AppButton&& other) noexcept;
  AppButton& operator=(AppButton&& other) noexcept;

  bool Contains(float x, float y) const override;
  void Draw(Graphics& graphics, BYTE global_alpha) const override;
  void LoadIcon();

  const std::wstring& GetTarget() const { return target_; }
  bool IsHovered() const override { return is_hovered_; }
  void SetHovered(bool hovered) override { is_hovered_ = hovered; }

 private:
  RectF bounds_;
  std::wstring target_;
  std::wstring name_;
  std::wstring icon_path_;
  mutable bool is_hovered_;
  Image* icon_image_;
};

#endif  // DOCK_BUTTON_H_
