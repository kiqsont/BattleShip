#include "ui/ShipPalette.h"

#include "core/Rules.h"
#include "ui/GameControls.h"
#include "ui/Pixmaps.h"
#include "ui/ShipDrag.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

constexpr int kPaletteWidth = 156;
constexpr int kTileHeight = 46;
constexpr int kTileCellPx = 16;  ///< 侧栏里的舰体是棋盘的缩小版
constexpr int kTilePadPx = 8;

/// 贴图里的船头朝向与 Direction 的语义相反：`Pixmaps.cpp` 里 ShipHead 原图朝上、
/// `Down` 为 0°，也就是"船体沿 Down 延伸、船头却朝上"。界面上要按**看起来船头朝哪**标字，
/// 所以 Up/Down、Left/Right 的文案要对换。
QString directionArrow(Direction direction)
{
    switch (direction) {
    case Direction::Up:    return QStringLiteral("↓");
    case Direction::Down:  return QStringLiteral("↑");
    case Direction::Left:  return QStringLiteral("→");
    case Direction::Right: return QStringLiteral("←");
    }
    return QStringLiteral("↑");
}

QString directionLabel(Direction direction)
{
    switch (direction) {
    case Direction::Up:    return QStringLiteral("↓ 下");
    case Direction::Down:  return QStringLiteral("↑ 上");
    case Direction::Left:  return QStringLiteral("→ 右");
    case Direction::Right: return QStringLiteral("← 左");
    }
    return QStringLiteral("↑ 上");
}

/// 1px 发丝分隔线：QFrame 的 HLine 走系统配色，没法跟主题，所以自己画。
/// 用 palette 填色而不是样式表：普通 QWidget 的样式表背景需要额外打开 WA_StyledBackground。
QWidget* makeDivider(QWidget* parent)
{
    auto* line = new QWidget(parent);
    line->setFixedHeight(1);
    line->setAutoFillBackground(true);
    QPalette palette = line->palette();
    palette.setColor(QPalette::Window, theme::paperDeep());
    line->setPalette(palette);
    return line;
}

} // namespace

// ---------------------------------------------------------------- ShipTile

ShipTile::ShipTile(ShipType type, QWidget* parent)
    : QWidget(parent)
    , m_type(type)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kTileHeight);
    setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::ArrowCursor);
    setToolTip(QStringLiteral("按住%1，拖到海域上放下").arg(shipTypeName(type)));
}

QSize ShipTile::sizeHint() const
{
    return QSize(kPaletteWidth - 20, kTileHeight);
}

void ShipTile::setDirection(Direction direction)
{
    if (m_direction == direction)
        return;
    m_direction = direction;
    update();
}

void ShipTile::setRemaining(int remaining)
{
    if (m_remaining == remaining)
        return;
    m_remaining = remaining;
    setCursor(remaining > 0 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    update();
}

void ShipTile::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const bool usable = m_remaining > 0;
    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    painter.setPen(Qt::NoPen);
    if (m_hovered && usable)
        painter.setBrush(QColor(0xb0, 0x38, 0x2a, 0x12));
    else
        painter.setBrush(theme::paper());
    painter.drawRoundedRect(box, 5, 5);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(m_hovered && usable ? theme::sealRed() : theme::hairline(), 1));
    painter.drawRoundedRect(box, 5, 5);

    // 放完的舰种整条压暗：一眼看出"这条已经没了"。
    painter.setOpacity(usable ? 1.0 : 0.35);

    theme::drawHeavyText(painter, QRect(kTilePadPx, 4, width() - 2 * kTilePadPx, 16),
                         shipTypeName(m_type), theme::bodyFont(12, QFont::DemiBold), theme::ink(),
                         0.4);
    theme::drawHeavyText(painter, QRect(0, 4, width() - kTilePadPx, 16),
                         QStringLiteral("剩 %1").arg(m_remaining), theme::bodyFont(11),
                         theme::muted(), 0.3, Qt::AlignRight | Qt::AlignVCenter);

    // 舰体永远横着画（4 格竖着放不进侧栏），朝向靠舰尾的小箭头示意。
    const int len = lengthOf(m_type);
    const int stripTop = height() - kTileCellPx - 6;
    for (int i = 0; i < len; ++i) {
        const QRect target(kTilePadPx + i * kTileCellPx, stripTop, kTileCellPx, kTileCellPx);
        const QPixmap& source = (i == 0) ? pixmaps::shipHead(Direction::Right)
                                        : pixmaps::shipBody();
        painter.drawPixmap(target, source.scaled(target.size(), Qt::IgnoreAspectRatio,
                                                Qt::SmoothTransformation));
    }

    const QRect arrowRect(kTilePadPx + len * kTileCellPx + 4, stripTop, 16, kTileCellPx);
    theme::drawHeavyText(painter, arrowRect, directionArrow(m_direction), theme::bodyFont(12),
                         theme::muted(), 0.3, Qt::AlignLeft | Qt::AlignVCenter);
}

void ShipTile::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_remaining > 0) {
        m_pressed = true;
        m_pressPos = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void ShipTile::mouseMoveEvent(QMouseEvent* event)
{
    if (m_pressed
        && (event->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()) {
        m_pressed = false;
        startDrag();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void ShipTile::mouseReleaseEvent(QMouseEvent* event)
{
    m_pressed = false;
    QWidget::mouseReleaseEvent(event);
}

void ShipTile::enterEvent(QEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void ShipTile::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ShipTile::startDrag()
{
    HullPayload payload;
    payload.type = m_type;
    payload.direction = m_direction;
    payload.fromBoard = false;

    // QDrag 由拖放系统接管并在结束时销毁，这里不持有它。
    auto* drag = new QDrag(this);
    drag->setMimeData(makeHullMime(payload));
    drag->setPixmap(renderHull(m_type, m_direction, kCellSizePx, 0.9));
    drag->setHotSpot(hullHotSpot(m_direction, lengthOf(m_type), 0, kCellSizePx));
    drag->exec(Qt::CopyAction);
}

// ------------------------------------------------------------ HullReturnZone

HullReturnZone::HullReturnZone(QWidget* parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(52);
    setToolTip(QStringLiteral("把海域上的舰拖到这里，整艘收回舰队"));
}

QSize HullReturnZone::sizeHint() const
{
    return QSize(kPaletteWidth - 20, 52);
}

void HullReturnZone::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const QColor color = m_hovered ? theme::sealRed() : theme::muted();

    painter.setPen(QPen(color, 1, Qt::DashLine));
    // 注意别写成 setBrush(m_hovered ? QColor(...) : Qt::NoBrush)：三目会把
    // Qt::NoBrush(0) 转成 QColor(Qt::color0) —— 也就是纯黑填充。
    if (m_hovered)
        painter.setBrush(QColor(0xb0, 0x38, 0x2a, 0x1a));
    else
        painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, 5, 5);

    theme::drawHeavyText(painter, rect(),
                         m_hovered ? QStringLiteral("松手收回") : QStringLiteral("拖回此处 · 收回"),
                         theme::bodyFont(11, QFont::DemiBold), color, 0.3, Qt::AlignCenter);
}

void HullReturnZone::dragEnterEvent(QDragEnterEvent* event)
{
    HullPayload payload;
    if (!readHullMime(event->mimeData(), &payload) || !payload.fromBoard) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    if (!m_hovered) {
        m_hovered = true;
        update();
    }
}

void HullReturnZone::dragMoveEvent(QDragMoveEvent* event)
{
    HullPayload payload;
    if (!readHullMime(event->mimeData(), &payload) || !payload.fromBoard) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
}

void HullReturnZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    QWidget::dragLeaveEvent(event);
    if (m_hovered) {
        m_hovered = false;
        update();
    }
}

void HullReturnZone::dropEvent(QDropEvent* event)
{
    HullPayload payload;
    if (!readHullMime(event->mimeData(), &payload) || !payload.fromBoard) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    m_hovered = false;
    update();
    emit hullReturned(payload.sourceHead);
}

// -------------------------------------------------------------- ShipPalette

ShipPalette::ShipPalette(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(kPaletteWidth);
    // 竖向也锁死：侧栏是内容驱动的高度，窗口拉高时不该跟着拉长（否则底部会留一大片空白）。
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto* caption = new QLabel(QStringLiteral("舰队"), this);
    styleLabel(caption, 12, true);
    layout->addWidget(caption);

    m_summary = new QLabel(this);
    styleLabel(m_summary, 11);
    layout->addWidget(m_summary);

    layout->addWidget(makeDivider(this));

    auto* dirLabel = new QLabel(QStringLiteral("方向 · 点一下向右转"), this);
    styleLabel(dirLabel, 11);
    layout->addWidget(dirLabel);

    // 一个按钮就够了：点一次顺时针转 90°（上 → 右 → 下 → 左），比四个按钮少一半点击。
    m_directionButton = new ThemeButton(directionLabel(Direction::Down), this,
                                        ThemeButton::Accent::Seal);
    m_directionButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_directionButton->setToolTip(QStringLiteral("点一下：上 → 右 → 下 → 左，循环"));
    connect(m_directionButton, &ThemeButton::clicked, this, &ShipPalette::rotateDirection);
    layout->addWidget(m_directionButton);

    layout->addWidget(makeDivider(this));

    for (int i = 0; i < kShipTypeCount; ++i) {
        auto* tile = new ShipTile(kAllShipTypes[static_cast<size_t>(i)], this);
        m_tiles[static_cast<size_t>(i)] = tile;
        layout->addWidget(tile);
    }

    layout->addStretch(1);

    m_returnZone = new HullReturnZone(this);
    connect(m_returnZone, &HullReturnZone::hullReturned, this, &ShipPalette::hullReturned);
    layout->addWidget(m_returnZone);

    setDirection(Direction::Down); // 显示「↑ 上」
    // 编制表是唯一来源：侧栏初始就按满编显示，PlacementView 之后再用实际剩余数覆盖。
    for (ShipType type : kAllShipTypes)
        setRemaining(type, rules::initialCount(type));
}

void ShipPalette::setSide(Side side)
{
    m_side = side;
    updateSummary();
}

void ShipPalette::setRemaining(ShipType type, int remaining)
{
    const size_t slot = static_cast<size_t>(type);
    m_remaining[slot] = remaining;
    if (ShipTile* tile = tileFor(type))
        tile->setRemaining(remaining);
    updateSummary();
}

ShipTile* ShipPalette::tileFor(ShipType type) const
{
    for (ShipTile* tile : m_tiles) {
        if (tile && tile->type() == type)
            return tile;
    }
    return nullptr;
}

void ShipPalette::setDirection(Direction direction)
{
    m_direction = direction;
    m_directionButton->setText(directionLabel(direction));
    for (ShipTile* tile : m_tiles) {
        if (tile)
            tile->setDirection(direction);
    }
}

void ShipPalette::rotateDirection()
{
    // 顺时针转 90°，按界面上看到的朝向：上 → 右 → 下 → 左。
    // 因为贴图朝向与枚举相反（见 directionLabel 注释），枚举里的顺序是
    // Down(上) → Left(右) → Up(下) → Right(左)。
    switch (m_direction) {
    case Direction::Down:  setDirection(Direction::Left); break;
    case Direction::Left:  setDirection(Direction::Up); break;
    case Direction::Up:    setDirection(Direction::Right); break;
    case Direction::Right: setDirection(Direction::Down); break;
    }
}

void ShipPalette::updateSummary()
{
    int remaining = 0;
    for (int count : m_remaining)
        remaining += count;

    const int total = rules::totalShipCount();
    m_summary->setText(QStringLiteral("%1 · 已放 %2/%3")
                           .arg(sideTitle(m_side))
                           .arg(total - remaining)
                           .arg(total));
}

void ShipPalette::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme::paper());
    painter.drawRoundedRect(box, 6, 6);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(theme::hairline(), 1));
    painter.drawRoundedRect(box, 6, 6);
}

} // namespace ui
} // namespace bs
