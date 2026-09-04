#ifndef DOCK_BUTTON_H_
#define DOCK_BUTTON_H_

#include <windows.h>
#include <string>

// Легковесная замена RectF для чистого GDI
struct Rect {
    int left, top, right, bottom;
    Rect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    int Width() const { return right - left; }
    int Height() const { return bottom - top; }
    bool Contains(int x, int y) const { return x >= left && x <= right && y >= top && y <= bottom; }
    void Inflate(int dx, int dy) { left -= dx; top -= dy; right += dx; bottom += dy; }
};

class Button {
public:
    virtual ~Button() = default;
    virtual bool Contains(int x, int y) const = 0;
    virtual void Draw(HDC hdc) const = 0;
    virtual bool IsHovered() const = 0;
    virtual void SetHovered(bool hovered) = 0;
};

class ControlButton : public Button {
public:
    enum class Type { kMinimize, kClose };
    explicit ControlButton(Rect bounds, Type type);
    bool Contains(int x, int y) const override;
    void Draw(HDC hdc) const override;
    Type GetType() const { return type_; }
    bool IsHovered() const override { return is_hovered_; }
    void SetHovered(bool hovered) override { is_hovered_ = hovered; }
private:
    Rect bounds_;
    Type type_;
    bool is_hovered_;
};

class AppButton : public Button {
public:
    AppButton(Rect bounds, std::wstring target, std::wstring name, std::wstring icon_path);
    ~AppButton() override;

    AppButton(const AppButton&) = delete;
    AppButton& operator=(const AppButton&) = delete;

    AppButton(AppButton&& other) noexcept;
    AppButton& operator=(AppButton&& other) noexcept;

    bool Contains(int x, int y) const override;
    void Draw(HDC hdc) const override;
    void LoadIcon();
    const std::wstring& GetTarget() const { return target_; }
    bool IsHovered() const override { return is_hovered_; }
    void SetHovered(bool hovered) override { is_hovered_ = hovered; }
private:
    Rect bounds_;
    std::wstring target_;
    std::wstring name_;
    std::wstring icon_path_;
    mutable bool is_hovered_;
    HICON hIcon_;
};

#endif  // DOCK_BUTTON_H_