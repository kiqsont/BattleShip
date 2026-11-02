#ifndef BS_UI_MENUBUTTON_H
#define BS_UI_MENUBUTTON_H

#include <QPushButton>

namespace bs {
namespace ui {

/// 主菜单条目：左侧序号、中间标题、右侧箭头，悬停时整行反白。
///
/// 继承 QPushButton 以保留 clicked 信号与无障碍语义，只接管绘制，
/// 因此布局、快捷键、焦点等行为仍由 Qt 提供。
class MenuButton : public QPushButton
{
    Q_OBJECT

public:
    explicit MenuButton(int index, const QString& text, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int m_index = 0;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_MENUBUTTON_H
