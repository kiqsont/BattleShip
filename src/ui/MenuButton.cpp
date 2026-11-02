#include "ui/MenuButton.h"

#include "ui/Theme.h"

#include <QPainter>
#include <QPen>

namespace bs {
namespace ui {
namespace {

constexpr int kRowHeight = 52;

} // namespace

MenuButton::MenuButton(int index, const QString& text, QWidget* parent)
    : QPushButton(text, parent)
    , m_index(index)
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

    const bool active = isEnabled() && (underMouse() || hasFocus() || isDown());
    const QRect row = rect().adjusted(0, 0, 0, -1);

    // 深色面板上，悬停整行反白；未选中时只留一条极淡的分隔线
    const QColor bright = theme::paper();
    if (active)
        painter.fillRect(row, bright);

    const QColor foreground = active ? theme::ink() : bright;
    const QColor numberColor = active ? QColor(22, 19, 15, 150) : QColor(250, 248, 242, 118);

    painter.setFont(theme::monoFont(11));
    painter.setPen(numberColor);
    painter.drawText(QRect(row.left(), row.top(), 26, row.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::number(m_index + 1).rightJustified(2, QLatin1Char('0')));

    // 深底白字额外补一道细描边，小字号才不会发虚
    theme::drawHeavyText(painter,
                         QRect(row.left() + 38, row.top(), row.width() - 38 - 38, row.height()),
                         text(), theme::titleFont(17), foreground, 0.5);

    if (isEnabled()) {
        const int tipX = row.right() - (active ? 16 : 20);
        const int tipY = row.center().y();
        painter.setPen(QPen(foreground, 1.5));
        painter.drawLine(tipX - 12, tipY, tipX, tipY);
        painter.drawLine(tipX - 5, tipY - 4, tipX, tipY);
        painter.drawLine(tipX - 5, tipY + 4, tipX, tipY);
    }

    if (!active) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(250, 248, 242, 34), 1));
        painter.drawLine(row.left(), row.bottom() + 1, row.right(), row.bottom() + 1);
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
