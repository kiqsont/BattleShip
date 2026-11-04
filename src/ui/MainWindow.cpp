#include "ui/MainWindow.h"

#include "ui/BattleView.h"
#include "ui/MenuPage.h"
#include "ui/PlacementView.h"
#include "ui/RulesView.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace bs {
namespace ui {
namespace {

/// 把定宽的页面套进居中的容器：窗口拉宽时两侧留白，内容不会被拉变形。
QWidget* wrapCentered(QWidget* page, int maxWidth)
{
    page->setMaximumWidth(maxWidth);

    auto* wrapper = new QWidget;
    auto* layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(page, 1);
    layout->addStretch(1);
    return wrapper;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    // 窗口可自由缩放：规则页按文档流重排，其余页面居中留白。
    // 最小尺寸取布置页"棋盘 + 舰队侧栏"的实际宽度，再小侧栏就会被挤扁。
    setMinimumSize(620, 660);
    resize(660, 900);
    setWindowTitle(QStringLiteral("Battle Ship"));

    m_stack = new QStackedWidget(this);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_stack);

    m_menuPage = new MenuPage(this);
    m_placement = new PlacementView(&m_session, this);
    m_battle = new BattleView(&m_session, this);
    m_rules = new RulesView(this);

    m_placementPage = wrapCentered(m_placement, 620);
    // 对战页固定竖排，铺满窗口宽度：两块 424px 棋盘需要完整宽度才不会出现水平滚动条。
    m_battlePage = m_battle;

    m_stack->addWidget(m_menuPage);
    m_stack->addWidget(m_placementPage);
    m_stack->addWidget(m_battlePage);
    m_stack->addWidget(m_rules);

    connect(m_menuPage, &MenuPage::startRequested, this, &MainWindow::startBattle);
    connect(m_menuPage, &MenuPage::placementRequested, this, &MainWindow::startPlacement);
    connect(m_menuPage, &MenuPage::rulesRequested, this, &MainWindow::startRules);

    connect(m_placement, &PlacementView::backRequested, this, &MainWindow::showMenu);
    connect(m_placement, &PlacementView::placementFinished, this, &MainWindow::showMenu);
    connect(m_rules, &RulesView::backRequested, this, &MainWindow::showMenu);
    connect(m_battle, &BattleView::exitRequested, this, [this] {
        m_session.leaveBattle();
        showMenu();
    });

    showMenu();
}

void MainWindow::paintEvent(QPaintEvent*)
{
    // 纸面只在最外层画一次：各页面自身透明，切换页面时底色不会跳。
    QPainter painter(this);
    theme::paintPaper(painter, rect());
}

void MainWindow::showMenu()
{
    m_stack->setCurrentWidget(m_menuPage);
}

void MainWindow::startPlacement(Side side)
{
    m_placement->setSide(side);
    m_stack->setCurrentWidget(m_placementPage);
}

void MainWindow::startBattle()
{
    m_battle->startBattle();
    m_stack->setCurrentWidget(m_battlePage);
}

void MainWindow::startRules()
{
    m_stack->setCurrentWidget(m_rules);
}

} // namespace ui
} // namespace bs
