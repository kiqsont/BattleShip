#include "ui/MenuButton.h"

#include "ui/Theme.h"

#include <QPainter>
#include <QPen>

namespace bs {
namespace ui {
namespace {

constexpr int kRowHeight = 50; ///< 按钮高度，圆角即半高，形成胶囊

} // namespace

MenuButton::MenuButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent)
{
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kRowHeight);
}

void MenuButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool hovered = underMouse() || isDown();
    const bool focused = hasFocus() && !isDown();

    const qreal r = height() / 2.0;
    const QRectF pill(rect());

    // 背景：常态半透明墨色，悬停/按下转朱红实色；叠在漫画上既清晰又不刺眼。
    painter.setPen(Qt::NoPen);
    painter.setBrush(hovered ? QBrush(theme::sealRed()) : QBrush(QColor(20, 17, 15, 110)));
    painter.drawRoundedRect(pill, r, r);

    // 极细描边，给常态一个轻边界，不显笨重。
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(250, 248, 242, hovered ? 0 : 40), 1));
    painter.drawRoundedRect(pill.adjusted(0.5, 0.5, -0.5, -0.5), r, r);

    // 文字：居中、字重略加；中文小字号补一道细描边才"实"。
    const QColor foreground = hovered ? theme::paper() : QColor(248, 246, 240, 238);
    theme::drawHeavyText(painter, rect(), text(), theme::bodyFont(18, QFont::Medium),
                         foreground, 0.5, Qt::AlignCenter);

    // 键盘聚焦时给一圈朱红描边（不依赖悬停）。
    if (focused) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(theme::sealRed(), 2));
        painter.drawRoundedRect(pill.adjusted(1, 1, -1, -1), r, r);
    }
}

void MenuButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    update();
}

void MenuButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    update();
}

} // namespace ui
} // namespace bs
