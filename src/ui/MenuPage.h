#ifndef BS_UI_MENUPAGE_H
#define BS_UI_MENUPAGE_H

#include "core/Types.h"

#include <QWidget>

class QBoxLayout;
class QVBoxLayout;

namespace bs {
namespace ui {

class MenuButton;
class MenuPanel;

/// 主菜单：漫画封面 + 深色操作面板。
///
/// 窄窗口（< 760px）竖排：封面在上、面板压在底部；
/// 宽窗口改成左右分栏：封面铺满左侧，面板占右侧，避免封面被放大到只剩天空。
/// 悬停整行反白，是常见的游戏主菜单做法，也和黑白漫画素材同一个调子。
class MenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit MenuPage(QWidget* parent = nullptr);

signals:
    void startRequested();
    void placementRequested(bs::Side side);
    void rulesRequested();
    void quitRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildPanel();

    QBoxLayout* m_layout = nullptr;
    MenuPanel* m_panel = nullptr;
    bool m_horizontal = false;
};

/// 底部/右侧操作面板：标题区 + 条目列表。背景与分隔线自行绘制。
class MenuPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MenuPanel(QWidget* parent = nullptr);

    MenuButton* addEntry(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QVBoxLayout* m_layout = nullptr;
    QVBoxLayout* m_entries = nullptr;
    int m_count = 0;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_MENUPAGE_H
