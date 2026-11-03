#ifndef BS_UI_MENUPAGE_H
#define BS_UI_MENUPAGE_H

#include "core/Types.h"

#include <QWidget>

class QBoxLayout;
class MenuButton;

namespace bs {
namespace ui {

/// 主菜单：漫画封面作背景，4 个悬浮按钮直接叠在画面上。
///
/// 窄窗口（< 760px）竖排：按钮压在底部的暗场，不挡住封面标题；
/// 宽窗口左右分栏：封面铺满左侧，按钮落在右侧暗场，避免封面被放大到只剩天空。
/// 背景绘制逻辑与原版保持一致（"背景图不变"）。
class MenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit MenuPage(QWidget* parent = nullptr);

signals:
    void startRequested();
    void placementRequested(bs::Side side);
    void rulesRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildButtons();

    QBoxLayout* m_layout = nullptr;
    QWidget* m_buttons = nullptr;
    bool m_horizontal = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_MENUPAGE_H
