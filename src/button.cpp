#include "button.h"

// ============================================================================
// CONTROL BUTTON IMPLEMENTATION
// ============================================================================
ControlButton::ControlButton(Rect bounds, Type type)
    : bounds_(bounds), type_(type), is_hovered_(false) {}

bool ControlButton::Contains(int x, int y) const {
    return bounds_.Contains(x, y);
}

void ControlButton::Draw(HDC hdc) const {
    if (is_hovered_) {
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(hdc, RGB(255, 255, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, bounds_.left, bounds_.top, bounds_.right, bounds_.bottom);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
    }

    COLORREF penColor = is_hovered_ ? RGB(255, 255, 255) : RGB(180, 180, 180);
    HPEN hCustomPen = CreatePen(PS_SOLID, 2, penColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hCustomPen);

    int cy = bounds_.top + bounds_.Height() / 2;
    int pad = 8;

    if (type_ == Type::kMinimize) {
        MoveToEx(hdc, bounds_.left + pad, cy, nullptr);
        LineTo(hdc, bounds_.right - pad, cy);
    }
    else {
        MoveToEx(hdc, bounds_.left + pad, bounds_.top + pad, nullptr);
        LineTo(hdc, bounds_.right - pad, bounds_.bottom - pad);
        MoveToEx(hdc, bounds_.right - pad, bounds_.top + pad, nullptr);
        LineTo(hdc, bounds_.left + pad, bounds_.bottom - pad);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hCustomPen);
}

// ============================================================================
// APP BUTTON IMPLEMENTATION
// ============================================================================
AppButton::AppButton(Rect bounds, std::wstring target, std::wstring name, std::wstring icon_path)
    : bounds_(bounds), target_(std::move(target)), name_(std::move(name)),
    icon_path_(std::move(icon_path)), is_hovered_(false), hIcon_(nullptr) {}

AppButton::~AppButton() {
    if (hIcon_) {
        DestroyIcon(hIcon_);
    }
}

AppButton::AppButton(AppButton&& other) noexcept
    : bounds_(other.bounds_), target_(std::move(other.target_)), name_(std::move(other.name_)),
    icon_path_(std::move(other.icon_path_)), is_hovered_(other.is_hovered_), hIcon_(other.hIcon_) {
    other.hIcon_ = nullptr;
}

AppButton& AppButton::operator=(AppButton&& other) noexcept {
    if (this != &other) {
        if (hIcon_) DestroyIcon(hIcon_);
        bounds_ = other.bounds_;
        target_ = std::move(other.target_);
        name_ = std::move(other.name_);
        icon_path_ = std::move(other.icon_path_);
        is_hovered_ = other.is_hovered_;
        hIcon_ = other.hIcon_;
        other.hIcon_ = nullptr;
    }
    return *this;
}

bool AppButton::Contains(int x, int y) const {
    return bounds_.Contains(x, y);
}

void AppButton::Draw(HDC hdc) const {
    if (is_hovered_) {
        Rect hover_rect = bounds_;
        hover_rect.Inflate(12, 12);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(hdc, RGB(230, 230, 230)); // Светлый фон для ховера
        HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, hover_rect.left, hover_rect.top, hover_rect.right, hover_rect.bottom);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
    }

    Rect icon_rect = bounds_;
    icon_rect.Inflate(-8, -8);
    if (is_hovered_) {
        icon_rect.Inflate(3, 3);
    }

    if (hIcon_) {
        DrawIconEx(hdc, icon_rect.left, icon_rect.top, hIcon_, icon_rect.Width(), icon_rect.Height(), 0, NULL, DI_NORMAL);
    }
    else {
        // Фолбэк: рисуем круг с первой буквой
        HBRUSH hBrush = CreateSolidBrush(RGB(100, 150, 255));
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 150, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        Ellipse(hdc, icon_rect.left, icon_rect.top, icon_rect.right, icon_rect.bottom);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        std::wstring letter = name_.substr(0, 1);
        RECT text_rect = { icon_rect.left, icon_rect.top, icon_rect.right, icon_rect.bottom };
        DrawTextW(hdc, letter.c_str(), -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
}

void AppButton::LoadIcon() {
    // Извлекаем иконку напрямую из целевого .exe файла (надежнее и красивее, чем загрузка PNG в чистом GDI)
    hIcon_ = ExtractIconW(nullptr, target_.c_str(), 0);
    if (!hIcon_) {
        // Фолбэк: пытаемся загрузить как отдельный .ico файл
        hIcon_ = (HICON)LoadImageW(nullptr, icon_path_.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
}