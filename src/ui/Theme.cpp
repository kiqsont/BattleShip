#include "ui/Theme.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QStringList>

namespace bs {
namespace ui {
namespace theme {
namespace {

const QColor kInk(0x16, 0x13, 0x0f);
const QColor kInkSoft(0x3f, 0x39, 0x32);
const QColor kMuted(0x8d, 0x85, 0x7a);
const QColor kPaper(0xfa, 0xf8, 0xf2);
const QColor kPaperDeep(0xd8, 0xd1, 0xc2);
const QColor kSeal(0xb0, 0x38, 0x2a);
const QColor kHairline(0x16, 0x13, 0x0f, 0x26);

/// 从候选列表里挑第一个系统已安装的字体，避免在不同机器上落到难看的默认字体。
QString firstAvailable(const QStringList& candidates, const QString& fallback)
{
    const QStringList families = QFontDatabase().families();
    for (const QString& candidate : candidates) {
        if (families.contains(candidate, Qt::CaseInsensitive))
            return candidate;
    }
    return fallback;
}

} // namespace

QColor ink() { return kInk; }
QColor inkSoft() { return kInkSoft; }
QColor muted() { return kMuted; }
QColor paper() { return kPaper; }
QColor paperDeep() { return kPaperDeep; }
QColor hairline() { return kHairline; }
QColor sealRed() { return kSeal; }

QString sansFamily()
{
    static const QString family = firstAvailable(
        {QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"),
         QStringLiteral("PingFang SC"), QStringLiteral("Noto Sans CJK SC"),
         QStringLiteral("Source Han Sans SC")},
        QStringLiteral("Microsoft YaHei"));
    return family;
}

QString monoFamily()
{
    static const QString family = firstAvailable(
        {QStringLiteral("Consolas"), QStringLiteral("JetBrains Mono"), QStringLiteral("Menlo"),
         QStringLiteral("DejaVu Sans Mono")},
        QStringLiteral("Courier New"));
    return family;
}

QFont displayFont(int pixelSize)
{
    QFont font(sansFamily());
    font.setPixelSize(pixelSize);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
    return font;
}

QFont titleFont(int pixelSize)
{
    QFont font(sansFamily());
    font.setPixelSize(pixelSize);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    return font;
}

QFont bodyFont(int pixelSize, int weight)
{
    QFont font(sansFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

QFont capsFont(int pixelSize)
{
    QFont font(sansFamily());
    font.setPixelSize(pixelSize);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 2.4);
    return font;
}

QFont monoFont(int pixelSize)
{
    QFont font(monoFamily());
    font.setPixelSize(pixelSize);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    return font;
}

void paintPaper(QPainter& painter, const QRect& rect)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    QLinearGradient wash(rect.topLeft(), rect.bottomLeft());
    wash.setColorAt(0.0, QColor(0xfd, 0xfc, 0xf8));
    wash.setColorAt(0.55, kPaper);
    wash.setColorAt(1.0, QColor(0xf6, 0xf2, 0xea));
    painter.fillRect(rect, wash);

    painter.restore();
}

void drawHeavyText(QPainter& painter, const QRect& rect, const QString& text, const QFont& font,
                   const QColor& color, qreal strokeWidth, Qt::Alignment alignment)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(font);

    const QFontMetricsF metrics(font);
    const qreal advance = metrics.horizontalAdvance(text);

    qreal x = rect.left();
    if (alignment & Qt::AlignHCenter)
        x = rect.left() + (rect.width() - advance) / 2.0;
    else if (alignment & Qt::AlignRight)
        x = rect.right() - advance;

    qreal y = rect.top() + metrics.ascent();
    if (alignment & Qt::AlignVCenter)
        y = rect.top() + (rect.height() + metrics.ascent() - metrics.descent()) / 2.0;
    else if (alignment & Qt::AlignBottom)
        y = rect.top() + rect.height() - metrics.descent();

    QPainterPath path;
    path.addText(x, y, font, text);

    painter.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(color);
    painter.drawPath(path);
    painter.restore();
}

} // namespace theme
} // namespace ui
} // namespace bs
