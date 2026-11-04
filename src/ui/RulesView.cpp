#include "ui/RulesView.h"

#include "core/Rules.h"
#include "core/Types.h"
#include "ui/Pixmaps.h"
#include "ui/Theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

/// 正文栏最大宽度。窗口再宽也不拉满，保证行长停在舒适阅读区间。
constexpr int kTextMaxWidth = 700;
/// 窄窗口下的最小左右边距。
constexpr int kSidePadding = 32;

QString rgba(int r, int g, int b, int a)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(a);
}

/// 随宽度缩放的插图。
///
/// 素材是白底灰度漫画，直接 drawPixmap 会在纸面上贴出一块白方块；
/// 这里用 Multiply 叠印，白底自然消失，像印在纸上一样。
class ScaledImage : public QWidget
{
public:
    explicit ScaledImage(const QString& path, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_source(path)
    {
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
        setMinimumWidth(160);
    }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override
    {
        if (m_source.isNull() || m_source.width() <= 0)
            return 0;
        return qRound(qreal(width) * m_source.height() / m_source.width());
    }

    QSize sizeHint() const override
    {
        const int width = qMin(m_source.width(), kTextMaxWidth);
        return QSize(width, heightForWidth(width));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), theme::paper());
        if (m_source.isNull())
            return;

        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.drawPixmap(rect(), m_source);
    }

private:
    QPixmap m_source;
};

/// 固定尺寸的小图标，同样用叠印去掉白底。
class IconTile : public QWidget
{
public:
    IconTile(const QPixmap& source, int size, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_source(source)
    {
        setFixedSize(size, size);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), theme::paper());
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.drawPixmap(rect(), m_source);
    }

private:
    QPixmap m_source;
};

QLabel* capsLabel(const QString& text, QWidget* parent = nullptr)
{
    auto* label = new QLabel(text, parent);
    label->setFont(theme::capsFont(10));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(theme::muted().name()));
    return label;
}

QWidget* hairline(QWidget* parent = nullptr)
{
    auto* line = new QFrame(parent);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background:%1; border:none;").arg(rgba(22, 19, 15, 38)));
    return line;
}

QLabel* makeParagraph(const QString& text, int pixelSize, QWidget* parent = nullptr)
{
    auto* label = new QLabel(parent);
    label->setTextFormat(Qt::RichText);
    label->setText(
        QStringLiteral("<div style=\"line-height:125%\">%1</div>").arg(text.toHtmlEscaped()));
    label->setWordWrap(true);
    label->setFont(theme::bodyFont(pixelSize));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(theme::inkSoft().name()));
    return label;
}

/// 项目符号用绘制的小方块，避免不同机器上缺字符变成方框。
class BulletDot : public QWidget
{
public:
    explicit BulletDot(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(12, 12);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::sealRed());
        painter.drawRect(QRect(2, 4, 5, 5));
    }
};

QWidget* makeBullet(const QString& text, QWidget* parent = nullptr)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    layout->addWidget(new BulletDot(row), 0, Qt::AlignTop);
    layout->addWidget(makeParagraph(text, 24, row), 1);
    return row;
}

QWidget* makeFigure(const QString& path, const QString& caption, QWidget* parent = nullptr)
{
    auto* block = new QWidget(parent);
    auto* layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(10);

    layout->addWidget(new ScaledImage(path, block));

    if (!caption.isEmpty()) {
        auto* label = new QLabel(caption, block);
        label->setFont(theme::bodyFont(16));
        label->setStyleSheet(QStringLiteral("color:%1;").arg(theme::muted().name()));
        label->setWordWrap(true);
        layout->addWidget(label);
    }
    return block;
}

QWidget* makeFleetTable(QWidget* parent = nullptr)
{
    auto* table = new QWidget(parent);
    // 宽窗口下表格不跟着拉满，否则"长度/数量"两列会飘到很远
    table->setMaximumWidth(560);

    auto* rows = new QVBoxLayout(table);
    rows->setContentsMargins(0, 6, 0, 0);
    rows->setSpacing(0);

    auto* header = new QWidget(table);
    auto* headRow = new QHBoxLayout(header);
    headRow->setContentsMargins(0, 0, 0, 8);
    headRow->setSpacing(12);
    headRow->addSpacing(38);
    headRow->addWidget(capsLabel(QStringLiteral("舰种"), header), 1);

    auto* lengthHead = capsLabel(QStringLiteral("长度"), header);
    lengthHead->setFixedWidth(70);
    lengthHead->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headRow->addWidget(lengthHead);

    auto* countHead = capsLabel(QStringLiteral("数量"), header);
    countHead->setFixedWidth(56);
    countHead->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headRow->addWidget(countHead);
    rows->addWidget(header);
    rows->addWidget(hairline(table));

    for (ShipType type : kAllShipTypes) {
        auto* row = new QWidget(table);
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 10, 0, 10);
        layout->setSpacing(12);

        layout->addWidget(new IconTile(pixmaps::shipIcon(type), 34, row));

        auto* name = new QLabel(shipTypeName(type), row);
        name->setFont(theme::titleFont(24));
        name->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
        layout->addWidget(name, 1);

        auto* length = new QLabel(QStringLiteral("%1 格").arg(lengthOf(type)), row);
        length->setFont(theme::monoFont(18));
        length->setStyleSheet(QStringLiteral("color:%1;").arg(theme::inkSoft().name()));
        length->setFixedWidth(70);
        length->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(length);

        auto* count = new QLabel(QStringLiteral("× %1").arg(rules::initialCount(type)), row);
        count->setFont(theme::monoFont(18));
        count->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
        count->setFixedWidth(56);
        count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(count);

        rows->addWidget(row);
        rows->addWidget(hairline(table));
    }

    return table;
}

QWidget* makeLegendItem(const QPixmap& pixmap, const QString& title, const QString& hint,
                        QWidget* parent)
{
    auto* item = new QWidget(parent);
    auto* layout = new QVBoxLayout(item);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    layout->addWidget(new IconTile(pixmap, 58, item), 0, Qt::AlignLeft);

    auto* label = new QLabel(title, item);
    label->setFont(theme::titleFont(18));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
    layout->addWidget(label);

    auto* sub = new QLabel(hint, item);
    sub->setFont(theme::bodyFont(16));
    sub->setStyleSheet(QStringLiteral("color:%1;").arg(theme::muted().name()));
    sub->setWordWrap(true);
    layout->addWidget(sub);

    return item;
}

QWidget* makeLegend(QWidget* parent = nullptr)
{
    auto* legend = new QWidget(parent);
    auto* layout = new QHBoxLayout(legend);
    layout->setContentsMargins(0, 10, 0, 4);
    layout->setSpacing(16);

    layout->addWidget(makeLegendItem(pixmaps::shipIcon(ShipType::Battleship),
                                     QStringLiteral("命中"), QStringLiteral("揭开并显示舰型"),
                                     legend), 1);
    layout->addWidget(makeLegendItem(pixmaps::volleyCell(), QStringLiteral("扫射命中"),
                                     QStringLiteral("只显示爆点"), legend), 1);
    layout->addWidget(makeLegendItem(pixmaps::missCell(), QStringLiteral("落空"),
                                     QStringLiteral("红叉，换对方"), legend), 1);
    return legend;
}

QWidget* makeTip(const QString& text, QWidget* parent = nullptr)
{
    auto* box = new QWidget(parent);
    box->setAttribute(Qt::WA_StyledBackground, true);
    box->setStyleSheet(QStringLiteral("background:%1; border:none; border-left:3px solid %2;")
                           .arg(rgba(22, 19, 15, 10), theme::ink().name()));

    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addWidget(makeParagraph(text, 23, box));
    return box;
}

struct Section {
    QWidget* root = nullptr;
    QVBoxLayout* body = nullptr;
};

Section makeSection(const QString& number, const QString& title, const QString& caps,
                    QWidget* parent)
{
    Section section;
    section.root = new QWidget(parent);

    auto* layout = new QVBoxLayout(section.root);
    layout->setContentsMargins(0, 40, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(hairline(section.root));
    layout->addSpacing(22);

    auto* head = new QHBoxLayout;
    head->setContentsMargins(0, 0, 0, 0);
    head->setSpacing(14);

    // 大号浅灰编号，是本页唯一的装饰元素
    auto* numeral = new QLabel(number, section.root);
    numeral->setFont(theme::titleFont(46));
    numeral->setStyleSheet(QStringLiteral("color:%1;").arg(theme::paperDeep().name()));
    head->addWidget(numeral, 0, Qt::AlignTop);

    auto* heading = new QVBoxLayout;
    heading->setSpacing(2);

    auto* titleLabel = new QLabel(title, section.root);
    titleLabel->setFont(theme::titleFont(30));
    titleLabel->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
    heading->addWidget(titleLabel);
    heading->addWidget(capsLabel(caps, section.root));
    head->addLayout(heading, 1);

    layout->addLayout(head);
    layout->addSpacing(16);

    section.body = new QVBoxLayout;
    section.body->setContentsMargins(0, 0, 0, 0);
    section.body->setSpacing(14);
    layout->addLayout(section.body);

    return section;
}

} // namespace

RulesView::RulesView(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    outer->addWidget(buildHeader());
    outer->addWidget(hairline(this));

    m_scroll = new QScrollArea(this);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_scroll->setStyleSheet(QStringLiteral(
                                "QScrollArea { background: transparent; }"
                                "QScrollBar:vertical { background: transparent; width: 10px;"
                                " margin: 0; }"
                                "QScrollBar::handle:vertical { background: %1; min-height: 48px;"
                                " border-radius: 5px; }"
                                "QScrollBar::handle:vertical:hover { background: %2; }"
                                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                                " height: 0; background: transparent; }"
                                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                                " background: transparent; }")
                                .arg(rgba(22, 19, 15, 60), rgba(22, 19, 15, 120)));
    m_scroll->setWidget(buildContent());

    outer->addWidget(m_scroll, 1);
}

QWidget* RulesView::buildHeader()
{
    auto* header = new QWidget(this);
    header->setFixedHeight(64);

    // 页眉与正文共用同一个左右边距，窗口拉宽时返回键仍贴着文字左边缘
    m_headerLayout = new QHBoxLayout(header);
    m_headerLayout->setContentsMargins(kSidePadding, 0, kSidePadding, 0);
    m_headerLayout->setSpacing(14);

    QHBoxLayout* row = m_headerLayout;

    m_back = new QPushButton(QStringLiteral("←"), header);
    m_back->setFixedSize(34, 34);
    m_back->setCursor(Qt::PointingHandCursor);
    m_back->setToolTip(QStringLiteral("返回主菜单"));
    m_back->setFont(theme::titleFont(16));
    m_back->setStyleSheet(QStringLiteral(
                              "QPushButton { border: 1px solid %1; background: transparent;"
                              " color: %1; padding-bottom: 2px; }"
                              "QPushButton:hover { background: %1; color: %2; }")
                              .arg(theme::ink().name(), theme::paper().name()));
    row->addWidget(m_back, 0, Qt::AlignVCenter);

    auto* title = new QLabel(QStringLiteral("规则手册"), header);
    title->setFont(theme::titleFont(16));
    title->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
    row->addWidget(title, 0, Qt::AlignVCenter);

    row->addStretch(1);
    row->addWidget(capsLabel(QStringLiteral("9 SHIPS · 100 CELLS"), header), 0, Qt::AlignVCenter);

    connect(m_back, &QPushButton::clicked, this, &RulesView::backRequested);
    return header;
}

QWidget* RulesView::buildContent()
{
    // 正文容器；左右边距由 updateColumnWidth() 按窗口宽度重算，实现"限宽 + 居中"
    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(kSidePadding, 26, kSidePadding, 64);
    layout->setSpacing(0);
    m_columnLayout = layout;

    // —— 卷首 ——
    layout->addWidget(capsLabel(QStringLiteral("BATTLE SHIP · RULES OF ENGAGEMENT"), column));
    layout->addSpacing(12);

    auto* title = new QLabel(QStringLiteral("玩法说明"), column);
    title->setFont(theme::displayFont(48));
    title->setStyleSheet(QStringLiteral("color:%1;").arg(theme::ink().name()));
    layout->addWidget(title);

    layout->addSpacing(14);
    layout->addWidget(makeParagraph(
        QStringLiteral("10 × 10 的海域，9 艘舰，两位玩家共用一台设备轮流开火。"
                       "谁先把对方的舰全部击沉，谁就获胜。"),
        18, column));
    layout->addSpacing(20);
    layout->addWidget(makeFigure(QStringLiteral(":/Image/RuleInstruction2.png"),
                                 QStringLiteral("「全軍覆沒的人就算贏」—— 漫画原作的规则页"), column));

    // —— 01 目标 ——
    {
        Section section = makeSection(QStringLiteral("01"), QStringLiteral("目标"),
                                      QStringLiteral("OBJECTIVE"), column);
        section.body->addWidget(makeParagraph(
            QStringLiteral("双方各持一片 10 × 10 海域，各布置 9 艘舰。"
                           "谁先把对方的舰全部击沉，谁就获胜。"),
            16, column));
        section.body->addWidget(makeBullet(
            QStringLiteral("轮到谁，就攻对方那片海域 —— 自己的海域藏着自己的舰，点它没有意义。"),
            column));
        section.body->addWidget(makeBullet(
            QStringLiteral("两位玩家共用同一台设备，分别以 Player1、Player2 的身份布置与出手。"),
            column));
        layout->addWidget(section.root);
    }

    // —— 02 舰队编制 ——
    {
        Section section = makeSection(QStringLiteral("02"), QStringLiteral("舰队编制"),
                                      QStringLiteral("FLEET"), column);
        section.body->addWidget(makeParagraph(
            QStringLiteral("四种舰占的格数不同：一艘舰的每一格都被揭开，才算击沉。"), 24, column));
        section.body->addWidget(makeFleetTable(column));
        section.body->addWidget(makeFigure(QStringLiteral(":/Image/RuleInstruction1.png"),
                                           QStringLiteral("从潜水舰到战舰，长度越长越难藏"), column));
        layout->addWidget(section.root);
    }

    // —— 03 布置海域 ——
    {
        Section section = makeSection(QStringLiteral("03"), QStringLiteral("布置海域"),
                                      QStringLiteral("DEPLOYMENT"), column);
        section.body->addWidget(makeParagraph(
            QStringLiteral("在主菜单选 Player1 或 Player2，进入自己的海域布置。"), 24, column));
        section.body->addWidget(makeBullet(
            QStringLiteral("先在右侧舰队面板选好「方向」，再按住舰体拖到海域上；"
                           "拖动时海域会给出半透明虚影，松手即落子。"),
            column));
        section.body->addWidget(makeBullet(
            QStringLiteral("放错了，直接在棋盘上按住那艘舰拖到别处即可换位置；"
                           "拖回侧栏底部的虚线框，整艘取回舰队。"), column));
        section.body->addWidget(makeBullet(
            QStringLiteral("九艘全部放完点「保存」；两方都布好之后，回主菜单点「开始」。"), column));
        section.body->addWidget(makeFigure(QStringLiteral(":/Image/RuleInstruction3.png"),
                                           QStringLiteral("「製作自己的艦隊」—— 布置时请对方回避"),
                                           column));
        layout->addWidget(section.root);
    }

    // —— 04 轮流进攻 ——
    {
        Section section = makeSection(QStringLiteral("04"), QStringLiteral("轮流进攻"),
                                      QStringLiteral("TRADING FIRE"), column);
        section.body->addWidget(makeParagraph(
            QStringLiteral("轮到你就点对方海域的一格开炮，只有两种结果："), 24, column));
        section.body->addWidget(makeBullet(QStringLiteral("命中：该格揭开，你继续进攻。"), column));
        section.body->addWidget(makeBullet(QStringLiteral("落空：换对方进攻。"), column));
        section.body->addWidget(makeBullet(
            QStringLiteral("击沉：一艘舰的所有格都被揭开时，对方失去它，"
                           "同时你获得一次「一同扫射」。"),
            column));
        section.body->addWidget(makeFigure(QStringLiteral(":/Image/RuleInstruction4.png"),
                                           QStringLiteral("「4 發才會沉」—— 命中一格不等于击沉"),
                                           column));
        layout->addWidget(section.root);
    }

    // —— 05 一同扫射 ——
    {
        Section section = makeSection(QStringLiteral("05"), QStringLiteral("一同扫射"),
                                      QStringLiteral("VOLLEY"), column);
        section.body->addWidget(makeParagraph(
            QStringLiteral("一次扫射覆盖 5 格，可以攒起来一次用完。"), 24, column));
        section.body->addWidget(makeBullet(
            QStringLiteral("在输入框填「次数」（每 1 次 = 5 格），点「瞄准」。"), column));
        section.body->addWidget(makeBullet(
            QStringLiteral("依次点选 5 × 次数 个格子，点「开火」发射。"), column));
        section.body->addWidget(makeBullet(
            QStringLiteral("扫射命中只显示爆点，不暴露舰型；只有击沉才显示船型。"), column));
        section.body->addWidget(makeFigure(QStringLiteral(":/Image/RuleInstruction5.png"),
                                           QStringLiteral("「擊沉 3 隻可得到 2 次射擊權利」—— 次数可累积"),
                                           column));
        layout->addWidget(section.root);
    }

    // —— 06 版面图例 ——
    {
        Section section = makeSection(QStringLiteral("06"), QStringLiteral("版面图例"),
                                      QStringLiteral("LEGEND"), column);
        section.body->addWidget(makeParagraph(QStringLiteral("海域上会出现的三种标记："),
                                              16, column));
        section.body->addWidget(makeLegend(column));
        section.body->addWidget(makeTip(
            QStringLiteral("同一格在一次对局里只会被记录一次，重复点不会生效。"
                           "想放弃瞄准，点行动条上的「取消」即可退回单发。"),
            column));
        layout->addWidget(section.root);
    }

    auto* end = capsLabel(QStringLiteral("— END —"), column);
    end->setAlignment(Qt::AlignCenter);
    layout->addSpacing(36);
    layout->addWidget(end);

    return column;
}

void RulesView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateColumnWidth();
}

void RulesView::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    // 滚动条出现后 viewport 会变窄，这里再校正一次
    updateColumnWidth();
}

void RulesView::updateColumnWidth()
{
    if (!m_columnLayout || !m_headerLayout || !m_scroll)
        return;

    const int available = m_scroll->viewport()->width();
    const int side = qMax(kSidePadding, (available - kTextMaxWidth) / 2);

    if (m_columnLayout->contentsMargins().left() == side)
        return;

    m_columnLayout->setContentsMargins(side, 26, side, 64);
    m_headerLayout->setContentsMargins(side, 0, side, 0);
}

} // namespace ui
} // namespace bs
