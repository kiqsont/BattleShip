#ifndef BS_UI_GAMECONTROLS_H
#define BS_UI_GAMECONTROLS_H

#include <QPushButton>

class QComboBox;
class QLabel;
class QLineEdit;

namespace bs {
namespace ui {

/// 游戏页统一的小型圆角按钮。
///
/// 主菜单的 MenuButton 是叠在漫画上的大胶囊，这里需要的是能塞进表格行、
/// 宽度固定的紧凑按钮，所以单独做一份；配色仍全部取自 theme::，保证同源。
/// 只重写 paintEvent，clicked / 焦点 / 键盘 / 无障碍全部沿用 QPushButton。
class ThemeButton : public QPushButton
{
    Q_OBJECT

public:
    /// Ink：纸面墨框，悬停转朱红；Seal：常态即朱红实色，用于主要动作。
    enum class Accent { Ink, Seal };

    explicit ThemeButton(const QString& text, QWidget* parent = nullptr,
                         Accent accent = Accent::Ink);

    void setAccent(Accent accent);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    Accent m_accent = Accent::Ink;
};

/// 只把样式设到控件自身，不下发到父容器，避免误伤 200 个棋盘格按钮。
void styleLineEdit(QLineEdit* edit);
void styleComboBox(QComboBox* combo);
void styleLabel(QLabel* label, int pixelSize = 14, bool strong = false);

} // namespace ui
} // namespace bs

#endif // BS_UI_GAMECONTROLS_H
