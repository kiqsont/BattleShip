#include "ui/CellButton.h"

#include "ui/Pixmaps.h"

#include <QMouseEvent>

namespace bs {
namespace ui {

CellButton::CellButton(Coord coord, QWidget* parent)
    : QPushButton(parent)
    , m_coord(coord)
{
    setFixedSize(kCellSizePx, kCellSizePx);
    setIconSize(QSize(kCellSizePx, kCellSizePx));
    setFocusPolicy(Qt::NoFocus);
    setFlat(false);
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

    setIcon(*pixmap);
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
