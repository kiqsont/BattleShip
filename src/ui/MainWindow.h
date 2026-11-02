#ifndef BS_UI_MAINWINDOW_H
#define BS_UI_MAINWINDOW_H

#include "core/GameSession.h"

#include <QWidget>

class QStackedWidget;

namespace bs {
namespace ui {

class BattleView;
class MenuPage;
class PlacementView;
class RulesView;

/// 主窗口：四个页面放在一个 QStackedWidget 里。
/// 旧版由 Widget 同时 new 出 Game / 两个 Configuration / RuleScene，
/// 彼此靠 hide()+show() 和复制 geometry 维持，谁也不能独立存在。
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void showMenu();
    void startPlacement(Side side);
    void startBattle();
    void startRules();

    GameSession m_session;
    QStackedWidget* m_stack = nullptr;
    MenuPage* m_menuPage = nullptr;
    PlacementView* m_placement = nullptr;
    BattleView* m_battle = nullptr;
    RulesView* m_rules = nullptr;
    QWidget* m_placementPage = nullptr;
    QWidget* m_battlePage = nullptr;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_MAINWINDOW_H
