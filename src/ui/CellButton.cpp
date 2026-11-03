#include "ui/CellButton.h"

#include "ui/Pixmaps.h"
#include "ui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>

namespace bs {
namespace ui {

CellButton::CellButton(Coord coord, QWidget* parent)
    : QPushButton(parent)
    , m_coord(coord)
{
    // 不再固定 40px：尺寸完全由 BoardWidget 的网格布局按可用空间分配，
    // 贴图在 paintEvent 里按当前 size() 缩放铺满，窗口缩放时棋盘跟着变。
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setMinimumSize(1, 1);
    setFocusPolicy(Qt::NoFocus);
    setFlat(true);
    setAttribute(Qt::WA_Hover, true);
    applyIcon();
}

void CellButton::setVisual(const CellVisual& visual)
{
    if (m_visual.kind == visual.kind && m_visual.ship == visual.ship
        && m_visual.direction == visual.direction && m_visual.sunk == visual.sunk) {
        return;
    }
    m_visual = visual;
    applyIcon();
}

void CellButton::setInteractive(bool interactive)
{
    if (m_interactive == interactive)
        return;
    m_interactive = interactive;
    setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (!interactive && m_hovered) {
        m_hovered = false;
        applyIcon();
    }
}

bool CellButton::canHover() const
{
    return m_interactive && m_visual.kind == CellVisual::Kind::Empty;
}

void CellButton::applyIcon()
{
    const QPixmap* pixmap = &pixmaps::emptyCell();

    if (canHover() && m_hovered) {
        pixmap = &pixmaps::traceCell();
    } else {
        switch (m_visual.kind) {
        case CellVisual::Kind::Empty:
            pixmap = &pixmaps::emptyCell();
            break;
        case CellVisual::Kind::Selected:
            pixmap = &pixmaps::traceCell();
            break;
        case CellVisual::Kind::Miss:
            pixmap = &pixmaps::missCell();
            break;
        case CellVisual::Kind::Hit:
            pixmap = &pixmaps::shipIcon(m_visual.ship);
            break;
        case CellVisual::Kind::VolleyHit:
            // 设计文档：一同扫射命中后不显示具体船只，除非该舰已被击沉
            pixmap = m_visual.sunk ? &pixmaps::shipIcon(m_visual.ship) : &pixmaps::volleyCell();
            break;
        case CellVisual::Kind::ShipHead:
            pixmap = &pixmaps::shipHead(m_visual.direction);
            break;
        case CellVisual::Kind::ShipBody:
            pixmap = &pixmaps::shipBody();
            break;
        }
    }

    m_pixmap = pixmap;
    updateScaledPixmap();
    update();
}

void CellButton::updateScaledPixmap()
{
    if (width() <= 0 || height() <= 0 || !m_pixmap || m_pixmap->isNull()) {
        m_scaled = QPixmap();
        return;
    }
    if (m_pixmap->size() == size()) {
        m_scaled = *m_pixmap;
        return;
    }
    m_scaled = m_pixmap->scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void CellButton::resizeEvent(QResizeEvent* event)
{
    QPushButton::resizeEvent(event);
    updateScaledPixmap();
}

void CellButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    if (!m_scaled.isNull()) {
        painter.drawPixmap(rect(), m_scaled);
    } else {
        painter.fillRect(rect(), theme::paper());
    }
    // 发丝边框把网格勾出来；贴图像素紧密拼接，线太粗会显脏。
    painter.setPen(QPen(theme::hairline(), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void CellButton::enterEvent(QEvent* event)
{
    if (canHover() && !m_hovered) {
        m_hovered = true;
        applyIcon();
    }
    QPushButton::enterEvent(event);
}

void CellButton::leaveEvent(QEvent* event)
{
    if (m_hovered) {
        m_hovered = false;
        applyIcon();
    }
    QPushButton::leaveEvent(event);
}

void CellButton::mousePressEvent(QMouseEvent* event)
{
    if (!m_interactive) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton)
        emit activated(m_coord);
    event->accept();
}

} // namespace ui
} // namespace bs
