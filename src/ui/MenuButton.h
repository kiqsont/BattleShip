#ifndef BS_UI_MENUBUTTON_H
#define BS_UI_MENUBUTTON_H

#include <QPushButton>

namespace bs {
namespace ui {

/// 主菜单按钮：半透明墨色圆角胶囊，悬停/按下时转朱红实色。
///
/// 继承 QPushButton 以保留 clicked 信号与无障碍语义，只接管绘制，
/// 因此布局、快捷键、焦点等行为仍由 Qt 提供。文字居中、字重略加。
class MenuButton : public QPushButton
{
    Q_OBJECT

public:
    explicit MenuButton(const QString& text, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_MENUBUTTON_H
