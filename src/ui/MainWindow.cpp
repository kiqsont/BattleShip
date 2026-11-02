#include "ui/MainWindow.h"

#include "ui/BattleView.h"
#include "ui/Pixmaps.h"
#include "ui/PlacementView.h"
#include "ui/RulesView.h"

#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

/// 主菜单页：背景图 + 竖排按钮。只有绘制，不需要信号，因此不必带 Q_OBJECT。
class MenuPage : public QWidget
{
public:
    explicit MenuPage(QWidget* parent = nullptr)
        : QWidget(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        const QPixmap& background = pixmaps::startBackground();
        if (!background.isNull())
            painter.drawPixmap(0, 0, width(), height(), background);
    }
};

QPushButton* makeMenuButton(const QString& text, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setFixedSize(110, 35);
    return button;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(480, 900);
    setWindowTitle(QStringLiteral("Battle Ship"));

    m_stack = new QStackedWidget(this);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_stack);

    m_menuPage = buildMenuPage();

    m_placement = new PlacementView(&m_session, this);
    m_battle = new BattleView(&m_session, this);
    m_rules = new RulesView(this);

    auto* rulesPage = new QWidget(this);
    auto* rulesLayout = new QVBoxLayout(rulesPage);
    rulesLayout->setContentsMargins(0, 0, 0, 0);
    rulesLayout->addWidget(m_rules, 0, Qt::AlignCenter);
    m_rulesPage = rulesPage;

    m_stack->addWidget(m_menuPage);
    m_stack->addWidget(m_placement);
    m_stack->addWidget(m_battle);
    m_stack->addWidget(rulesPage);

    connect(m_placement, &PlacementView::backRequested, this, &MainWindow::showMenu);
    connect(m_placement, &PlacementView::placementFinished, this, &MainWindow::showMenu);
    connect(m_rules, &RulesView::backRequested, this, &MainWindow::showMenu);
    connect(m_battle, &BattleView::exitRequested, this, [this] {
        m_session.leaveBattle();
        showMenu();
    });

    showMenu();
}

QWidget* MainWindow::buildMenuPage()
{
    auto* page = new MenuPage(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    layout->addStretch(1);

    auto* start = makeMenuButton(QStringLiteral("开始"), page);
    auto* player1 = makeMenuButton(QStringLiteral("Player1"), page);
    auto* player2 = makeMenuButton(QStringLiteral("Player2"), page);
    auto* instruction = makeMenuButton(QStringLiteral("规则说明"), page);
    auto* quit = makeMenuButton(QStringLiteral("退出"), page);

    for (QPushButton* button : {start, player1, player2, instruction, quit})
        layout->addWidget(button, 0, Qt::AlignHCenter);

    layout->addStretch(1);

    connect(start, &QPushButton::clicked, this, &MainWindow::startBattle);
    connect(player1, &QPushButton::clicked, this, [this] { startPlacement(Side::Player1); });
    connect(player2, &QPushButton::clicked, this, [this] { startPlacement(Side::Player2); });
    connect(instruction, &QPushButton::clicked, this,
            [this] { m_stack->setCurrentWidget(m_rulesPage); });
    connect(quit, &QPushButton::clicked, this, &MainWindow::close);

    return page;
}

void MainWindow::showMenu()
{
    m_stack->setCurrentWidget(m_menuPage);
}

void MainWindow::startPlacement(Side side)
{
    m_placement->setSide(side);
    m_stack->setCurrentWidget(m_placement);
}

void MainWindow::startBattle()
{
    m_battle->startBattle();
    m_stack->setCurrentWidget(m_battle);
}

} // namespace ui
} // namespace bs
