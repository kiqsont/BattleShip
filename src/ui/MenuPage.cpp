#include "ui/MenuPage.h"

#include "ui/MenuButton.h"
#include "ui/Pixmaps.h"
#include "ui/Theme.h"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QResizeEvent>

namespace bs {
namespace ui {
namespace {

constexpr int kPanelMaxWidth = 460; ///< 条目最长宽度，再宽就不好扫视了
constexpr int kSidePadding = 34;    ///< 面板内容的最小左右边距
constexpr int kWideThreshold = 760; ///< 超过这个宽度改成左右分栏
constexpr int kPanelShare = 44;     ///< 分栏时右侧面板占宽度的百分比

QString white(int alpha = 255)
{
    return QStringLiteral("rgba(250, 248, 242, %1)").arg(alpha);
}

} // namespace

MenuPanel::MenuPanel(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(kSidePadding, 30, kSidePadding, 28);
    m_layout->setSpacing(0);

    // 分栏模式下内容需要垂直居中，两端各留一段弹性空白；
    // 竖排模式下面板高度由内容决定，这两段不起作用。
    m_layout->addStretch(0);

    auto* caps = new QLabel(QStringLiteral("NAVAL COMBAT · 10 × 10"), this);
    caps->setFont(theme::capsFont(10));
    caps->setStyleSheet(QStringLiteral("color:%1;").arg(white(120)));
    m_layout->addWidget(caps);

    m_layout->addSpacing(10);

    auto* title = new QLabel(QStringLiteral("海 战 棋"), this);
    title->setFont(theme::displayFont(32));
    title->setStyleSheet(QStringLiteral("color:%1;").arg(white()));
    m_layout->addWidget(title);

    m_layout->addSpacing(14);

    auto* bar = new QFrame(this);
    bar->setFixedSize(46, 3);
    bar->setStyleSheet(QStringLiteral("background:%1; border:none;").arg(theme::sealRed().name()));
    m_layout->addWidget(bar);

    m_layout->addSpacing(22);

    m_entries = new QVBoxLayout;
    m_entries->setContentsMargins(0, 0, 0, 0);
    m_entries->setSpacing(0);
    m_layout->addLayout(m_entries);

    m_layout->addStretch(0);
}

MenuButton* MenuPanel::addEntry(const QString& text)
{
    auto* button = new MenuButton(m_count++, text, this);
    m_entries->addWidget(button);
    return button;
}

void MenuPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 宽窗口下把内容收到中间，条目不会被拉成一整行
    const int side = qMax(kSidePadding, (width() - kPanelMaxWidth) / 2);
    if (m_layout->contentsMargins().left() != side)
        m_layout->setContentsMargins(side, 30, side, 28);
}

void MenuPanel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::ink());

    // 与封面相接的那条极细亮线：竖排在上面，分栏在左侧
    painter.setPen(QPen(QColor(250, 248, 242, 40), 1));
    if (height() >= width())
        painter.drawLine(0, 0, 0, height());
    else
        painter.drawLine(0, 0, width(), 0);
}

MenuPage::MenuPage(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addStretch(1);

    buildPanel();
    m_layout->addWidget(m_panel, 0);
}

void MenuPage::buildPanel()
{
    m_panel = new MenuPanel(this);

    MenuButton* start = m_panel->addEntry(QStringLiteral("开始"));
    MenuButton* player1 = m_panel->addEntry(QStringLiteral("Player1"));
    MenuButton* player2 = m_panel->addEntry(QStringLiteral("Player2"));
    MenuButton* rules = m_panel->addEntry(QStringLiteral("规则说明"));
    MenuButton* quit = m_panel->addEntry(QStringLiteral("退出"));

    connect(start, &QPushButton::clicked, this, &MenuPage::startRequested);
    connect(player1, &QPushButton::clicked, this,
            [this] { emit placementRequested(Side::Player1); });
    connect(player2, &QPushButton::clicked, this,
            [this] { emit placementRequested(Side::Player2); });
    connect(rules, &QPushButton::clicked, this, &MenuPage::rulesRequested);
    connect(quit, &QPushButton::clicked, this, &MenuPage::quitRequested);
}

void MenuPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    const bool wide = width() >= kWideThreshold;
    if (wide != m_horizontal) {
        m_horizontal = wide;
        m_layout->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    }

    if (wide) {
        m_layout->setStretch(0, 1);
        m_layout->setStretch(1, 0);
        m_panel->setFixedWidth(qBound(360, width() * kPanelShare / 100, 520));
    } else if (m_panel->minimumWidth() != 0) {
        m_panel->setMinimumWidth(0);
        m_panel->setMaximumWidth(QWIDGETSIZE_MAX);
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
        // 左右分栏：封面铺满面板左侧那片区域，避免放大过头导致画面发糊
        const int panelLeft = qMax(1, m_panel->geometry().left());
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
