#include "button.h"

// ============================================================================
// CONTROL BUTTON IMPLEMENTATION
// ============================================================================

ControlButton::ControlButton(RectF bounds, Type type)
    : bounds_(bounds), type_(type), is_hovered_(false) {}

bool ControlButton::Contains(float x, float y) const {
  return bounds_.Contains(x, y);
}

void ControlButton::Draw(Graphics& graphics, BYTE global_alpha) const {
  Color btn_color =
      is_hovered_ ? Color(255, 255, 255, 255) : Color(global_alpha, 180, 180, 180);

  if (is_hovered_) {
    SolidBrush hover_brush(Color(60, 255, 255, 255));
    graphics.FillEllipse(&hover_brush, bounds_);
  }

  Pen icon_pen(btn_color, 2.0f);
  if (type_ == Type::kMinimize) {
    graphics.DrawLine(&icon_pen, bounds_.X + 8, bounds_.Y + bounds_.Height / 2,
                      bounds_.X + bounds_.Width - 8,
                      bounds_.Y + bounds_.Height / 2);
  } else {
    graphics.DrawLine(&icon_pen, bounds_.X + 8, bounds_.Y + 8,
                      bounds_.X + bounds_.Width - 8,
                      bounds_.Y + bounds_.Height - 8);
    graphics.DrawLine(&icon_pen, bounds_.X + bounds_.Width - 8, bounds_.Y + 8,
                      bounds_.X + 8, bounds_.Y + bounds_.Height - 8);
  }
}

// ============================================================================
// APP BUTTON IMPLEMENTATION
// ============================================================================

AppButton::AppButton(RectF bounds, std::wstring target, std::wstring name,
                     std::wstring icon_path)
    : bounds_(bounds),
      target_(std::move(target)),
      name_(std::move(name)),
      icon_path_(std::move(icon_path)),
      is_hovered_(false),
      icon_image_(nullptr) {}

AppButton::~AppButton() {
  if (icon_image_) {
    delete icon_image_;
  }
}

AppButton::AppButton(AppButton&& other) noexcept
    : bounds_(other.bounds_),
      target_(std::move(other.target_)),
      name_(std::move(other.name_)),
      icon_path_(std::move(other.icon_path_)),
      is_hovered_(other.is_hovered_),
      icon_image_(other.icon_image_) {
  other.icon_image_ = nullptr;
}

AppButton& AppButton::operator=(AppButton&& other) noexcept {
  if (this != &other) {
    if (icon_image_) delete icon_image_;
    bounds_ = other.bounds_;
    target_ = std::move(other.target_);
    name_ = std::move(other.name_);
    icon_path_ = std::move(other.icon_path_);
    is_hovered_ = other.is_hovered_;
    icon_image_ = other.icon_image_;
    other.icon_image_ = nullptr;
  }
  return *this;
}

bool AppButton::Contains(float x, float y) const {
  return bounds_.Contains(x, y);
}

void AppButton::Draw(Graphics& graphics, BYTE global_alpha) const {
  if (is_hovered_) {
    RectF hover_rect = bounds_;
    hover_rect.Inflate(12.0f, 12.0f);
    GraphicsPath hover_path;
    hover_path.AddEllipse(hover_rect);
    SolidBrush hover_brush(Color(80, 255, 255, 255));
    graphics.FillPath(&hover_brush, &hover_path);
  }

  RectF icon_rect = bounds_;
  icon_rect.Inflate(-8.0f, -8.0f);
  if (is_hovered_) {
    icon_rect.Inflate(3.0f, 3.0f);
  }

  if (icon_image_ && icon_image_->GetLastStatus() == Ok) {
    graphics.DrawImage(icon_image_, icon_rect);
  } else {
    GraphicsPath icon_path;
    icon_path.AddEllipse(icon_rect);
    SolidBrush icon_brush(Color(global_alpha, 100, 150, 255));
    graphics.FillPath(&icon_brush, &icon_path);

    FontFamily font_family(L"Segoe UI");
    Font font(&font_family, 20.0f, FontStyleBold, UnitPixel);
    SolidBrush text_brush(Color(global_alpha, 255, 255, 255));
    StringFormat string_format;
    string_format.SetAlignment(StringAlignmentCenter);
    string_format.SetLineAlignment(StringAlignmentCenter);

    std::wstring letter = name_.substr(0, 1);
    graphics.DrawString(letter.c_str(), -1, &font, icon_rect, &string_format,
                        &text_brush);
  }
}

void AppButton::LoadIcon() {
  icon_image_ = Image::FromFile(icon_path_.c_str());
}
