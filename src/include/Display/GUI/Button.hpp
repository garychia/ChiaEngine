#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "Data/String.hpp"
#include "IInteractable.hpp"
#include "System/Operation/Event.hpp"

class Button : public IInteractable
{
  private:
    String label;       // 按鈕文字(空 = 不畫 label)
    float fontSize;     // 字形像素高度
    Color textColor;    // 文字顏色(與幾何顏色 SetColor 獨立)

  public:
    Event<void()> pressEvent;

    Event<void()> releaseEvent;

    Event<void()> clickEvent;

    Event<void()> hoverEvent;

    Button(const Point2D &windowSize, const Border &border);

    virtual void OnMouseDown(const Point2D &coordinates) override;

    virtual void OnMouseUp(const Point2D &coordinates) override;

    virtual void OnClicked(const Point2D &coordinates) override;

    virtual void OnHovered(const Point2D &coordinates) override;

    void SetLabel(const String &text);

    const String &GetLabel() const;

    void SetFontSize(float size);

    float GetFontSize() const;

    void SetTextColor(const Color &color);

    const Color &GetTextColor() const;
};

#endif // BUTTON_HPP
