#include "ui/GameControls.h"

#include "ui/Theme.h"

#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QPen>

namespace bs {
namespace ui {
namespace {

constexpr qreal kRadius = 6.0;

QString argb(const QColor& color) { return color.name(QColor::HexArgb); }

} // namespace

ThemeButton::ThemeButton(const QString& text, QWidget* parent, Accent accent)
    : QPushButton(text, parent)
    , m_accent(accent)
{
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setFixedHeight(30);
}

void ThemeButton::setAccent(Accent accent)
{
    if (m_accent == accent)
        return;
    m_accent = accent;
    update();
}

void ThemeButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool hovered = (underMouse() || isDown()) && isEnabled();
    const bool focused = hasFocus() && !isDown() && isEnabled();
    const bool seal = m_accent == Accent::Seal;

    QColor background;
    QColor foreground;
    QColor border;

    if (!isEnabled()) {
        background = theme::paperDeep();
        foreground = theme::muted();
        border = theme::hairline();
    } else if (seal) {
        background = hovered ? QColor(0x8c, 0x2b, 0x20) : theme::sealRed();
        foreground = theme::paper();
        border = background;
    } else if (hovered) {
        background = theme::sealRed();
        foreground = theme::paper();
        border = theme::sealRed();
    } else {
        background = theme::paper();
        foreground = theme::ink();
        border = theme::hairline();
    }

    const QRectF box(rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(box, kRadius, kRadius);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(border, 1));
    painter.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), kRadius, kRadius);

    theme::drawHeavyText(painter, rect(), text(), theme::bodyFont(14, QFont::Medium), foreground,
                         0.45, Qt::AlignCenter);

    if (focused) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(theme::sealRed(), 2));
        painter.drawRoundedRect(box.adjusted(1, 1, -1, -1), kRadius, kRadius);
    }
}

void ThemeButton::enterEvent(QEvent* event)
{
    QPushButton::enterEvent(event);
    update();
}

void ThemeButton::leaveEvent(QEvent* event)
{
    QPushButton::leaveEvent(event);
    update();
}

void styleLineEdit(QLineEdit* edit)
{
    edit->setFont(theme::bodyFont(14));
    edit->setStyleSheet(
        QStringLiteral("QLineEdit { background: %1; color: %2; border: 1px solid %3;"
                       " border-radius: 4px; padding: 2px 4px;"
                       " selection-background-color: %4; selection-color: %1; }"
                       " QLineEdit:focus { border-color: %4; }"
                       " QLineEdit:disabled { background: %5; color: %6; }")
            .arg(theme::paper().name(), theme::ink().name(), argb(theme::hairline()),
                 theme::sealRed().name(), theme::paperDeep().name(), theme::muted().name()));
}

void styleComboBox(QComboBox* combo)
{
    combo->setFont(theme::bodyFont(14));
    combo->setStyleSheet(
        QStringLiteral("QComboBox { background: %1; color: %2; border: 1px solid %3;"
                       " border-radius: 4px; padding: 2px 6px; min-height: 22px; }"
                       " QComboBox:focus { border-color: %4; }"
                       " QComboBox:disabled { background: %5; color: %6; }"
                       " QComboBox::drop-down { border: 0px; width: 18px; }"
                       " QComboBox QAbstractItemView { background: %1; color: %2;"
                       " selection-background-color: %4; selection-color: %1; }")
            .arg(theme::paper().name(), theme::ink().name(), argb(theme::hairline()),
                 theme::sealRed().name(), theme::paperDeep().name(), theme::muted().name()));
}

void styleLabel(QLabel* label, int pixelSize, bool strong)
{
    label->setFont(theme::bodyFont(pixelSize, strong ? QFont::DemiBold : QFont::Normal));
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, strong ? theme::ink() : theme::inkSoft());
    label->setPalette(palette);
}

} // namespace ui
} // namespace bs
