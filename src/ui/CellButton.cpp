#include "ui/CellButton.h"

#include "ui/Pixmaps.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>

namespace bs {
namespace ui {
namespace {

/// 虚影：半透明贴在格子上，合不合法只靠颜色区分（墨 = 可以放，朱红 = 放不下）。
constexpr qreal kGhostOpacity = 0.55;
constexpr int kGhostTintOk = 26;
constexpr int kGhostTintBad = 78;

} // namespace

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
        && m_visual.direction == visual.direction && m_visual.valid == visual.valid) {
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
    updateCursor();
    if (!interactive && m_hovered) {
        m_hovered = false;
        applyIcon();
    }
}

void CellButton::setDragSource(bool draggable)
{
    if (m_dragSource == draggable)
        return;
    m_dragSource = draggable;
    updateCursor();
}

void CellButton::updateCursor()
{
    if (m_interactive)
        setCursor(Qt::PointingHandCursor);
    else if (m_dragSource)
        setCursor(Qt::OpenHandCursor);
    else
        setCursor(Qt::ArrowCursor);
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
        case CellVisual::Kind::VolleyCover:
            // 与初版 button::shoot() 一致：一同扫射打过的格子统一盖 togeShoot.png，
            // 不论命中、击沉还是落空，都不把格子里的战舰内容显示出来。
            pixmap = &pixmaps::volleyCell();
            break;
        case CellVisual::Kind::ShipHead:
        case CellVisual::Kind::GhostHead:
            pixmap = &pixmaps::shipHead(m_visual.direction);
            break;
        case CellVisual::Kind::ShipBody:
        case CellVisual::Kind::GhostBody:
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
    const bool ghost = m_visual.isGhost();

    if (!m_scaled.isNull()) {
        // 虚影用半透明画：底下的海水/已有舰船还能透出来，一眼能看出"这是预演"。
        painter.setOpacity(ghost ? kGhostOpacity : 1.0);
        painter.drawPixmap(rect(), m_scaled);
        painter.setOpacity(1.0);
    } else {
        painter.fillRect(rect(), theme::paper());
    }

    if (ghost) {
        QColor tint = m_visual.valid ? theme::ink() : theme::sealRed();
        tint.setAlpha(m_visual.valid ? kGhostTintOk : kGhostTintBad);
        painter.fillRect(rect(), tint);
    }

    // 发丝边框把网格勾出来；贴图像素紧密拼接，线太粗会显脏。
    painter.setPen(QPen(m_visual.isGhost() && !m_visual.valid ? theme::sealRed() : theme::hairline(),
                        m_visual.isGhost() && !m_visual.valid ? 2 : 1));
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
    // 拖走整艘舰：即使这格不响应点击（布置阶段），也要能按住拖。
    // 这里必须 accept 而不是 ignore —— 被 ignore 的按下会转交给父控件处理，
    // 之后的移动事件就归父控件了，格子再也收不到 mouseMove，拖拽根本起不来。
    if (m_dragSource && event->button() == Qt::LeftButton) {
        m_dragArmed = true;
        m_dragPressPos = event->pos();
        event->accept();
        return;
    }

    if (!m_interactive) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton)
        emit activated(m_coord);
    event->accept();
}

void CellButton::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragArmed
        && (event->pos() - m_dragPressPos).manhattanLength() >= QApplication::startDragDistance()) {
        m_dragArmed = false;
        emit dragRequested(m_coord);
        return;
    }
    QPushButton::mouseMoveEvent(event);
}

void CellButton::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragArmed = false;
    QPushButton::mouseReleaseEvent(event);
}

} // namespace ui
} // namespace bs
