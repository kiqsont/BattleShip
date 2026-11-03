#include "ui/MenuPage.h"

#include "ui/MenuButton.h"
#include "ui/Pixmaps.h"
#include "ui/Theme.h"

#include <QBoxLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QResizeEvent>

namespace bs {
namespace ui {
namespace {

constexpr int kWideThreshold = 760;      ///< 超过这个宽度改成左右分栏
constexpr int kPanelShare = 44;          ///< 分栏时右侧暗场占宽度的百分比
constexpr int kVerticalButtonWidth = 280; ///< 竖排时按钮列宽度
constexpr int kColumnPad = 36;           ///< 分栏时按钮两侧内边距
constexpr int kBottomMargin = 64;        ///< 竖排时按钮列离底边的距离

} // namespace

MenuPage::MenuPage(QWidget* parent)
    : QWidget(parent)
{
    // 竖排时：上方留弹性空白把按钮列压到底部暗场；宽窗口时整体改为左右分栏。
    m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addStretch(1);

    buildButtons();
    m_layout->addWidget(m_buttons, 0, Qt::AlignHCenter);
}

void MenuPage::buildButtons()
{
    m_buttons = new QWidget(this);
    auto* col = new QVBoxLayout(m_buttons);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(14);
    col->addStretch(1);

    MenuButton* start = new MenuButton(QStringLiteral("开始"), m_buttons);
    MenuButton* player1 = new MenuButton(QStringLiteral("Player1"), m_buttons);
    MenuButton* player2 = new MenuButton(QStringLiteral("Player2"), m_buttons);
    MenuButton* rules = new MenuButton(QStringLiteral("规则说明"), m_buttons);

    col->addWidget(start);
    col->addWidget(player1);
    col->addWidget(player2);
    col->addWidget(rules);

    col->addStretch(1);

    connect(start, &QPushButton::clicked, this, &MenuPage::startRequested);
    connect(player1, &QPushButton::clicked, this,
            [this] { emit placementRequested(Side::Player1); });
    connect(player2, &QPushButton::clicked, this,
            [this] { emit placementRequested(Side::Player2); });
    connect(rules, &QPushButton::clicked, this, &MenuPage::rulesRequested);
}

void MenuPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    const bool wide = width() >= kWideThreshold;
    if (wide != m_horizontal) {
        m_horizontal = wide;
        m_layout->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    }

    auto* col = qobject_cast<QVBoxLayout*>(m_buttons->layout());
    if (wide) {
        m_layout->setStretch(0, 1);
        m_layout->setStretch(1, 0);
        const int panelWidth = qBound(360, width() * kPanelShare / 100, 520);
        m_buttons->setFixedWidth(panelWidth);
        if (col)
            col->setContentsMargins(kColumnPad, 0, kColumnPad, 0);
    } else {
        m_buttons->setFixedWidth(kVerticalButtonWidth);
        if (col)
            col->setContentsMargins(0, 0, 0, kBottomMargin);
    }

    update();
}

void MenuPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::ink());

    const QPixmap& art = pixmaps::startBackground();
    if (art.isNull())
        return;

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_horizontal) {
        // 左右分栏：封面铺满按钮列左侧那片区域，避免放大过头导致画面发糊
        const int panelLeft = qMax(1, m_buttons->geometry().left());
        const qreal scale = qMax(qreal(height()) / art.height(),
                                 qreal(panelLeft + 8) / art.width());
        const QSize target = art.size() * scale;
        const QRect plate(0, (height() - target.height()) / 2, target.width(), target.height());
        painter.drawPixmap(plate, art);

        // 右侧压一道渐隐，封面不会在面板边缘被"一刀切"
        const int fade = qMin(140, panelLeft);
        QLinearGradient blend(panelLeft - fade, 0, panelLeft, 0);
        blend.setColorAt(0.0, QColor(22, 19, 15, 0));
        blend.setColorAt(1.0, theme::ink());
        painter.fillRect(QRect(panelLeft - fade, 0, fade, height()), blend);
        return;
    }

    // 竖排：按宽度铺满、贴顶，保证两侧竖排标题完整；多出来的高度渐隐进墨色
    const QSize target = art.size() * (qreal(width()) / art.width());
    const QRect plate(0, 0, target.width(), target.height());
    painter.drawPixmap(plate, art);

    const int visibleBottom = qMin(height(), plate.bottom() + 1);
    const int fadeHeight = qMin(180, visibleBottom);
    QLinearGradient fade(0, visibleBottom - fadeHeight, 0, visibleBottom);
    fade.setColorAt(0.0, QColor(22, 19, 15, 0));
    fade.setColorAt(1.0, QColor(22, 19, 15, 215));
    painter.fillRect(QRect(0, visibleBottom - fadeHeight, width(), fadeHeight), fade);
}

} // namespace ui
} // namespace bs
