#ifndef BS_UI_TURNBAR_H
#define BS_UI_TURNBAR_H

#include "core/Types.h"

#include <QWidget>

class QLabel;
class QLineEdit;

namespace bs {
namespace ui {

class ThemeButton;

/// 一侧玩家的紧凑行动条：扫射次数、本次使用次数、瞄准 / 开火 / 取消、命中与选格反馈。
/// 单行排布、固定高度 36px，避免在对战页里占掉两块棋盘之间的版面。
class TurnBar : public QWidget
{
    Q_OBJECT

public:
    explicit TurnBar(Side side, QWidget* parent = nullptr);

    void setVolleyCredits(int credits);
    void setActive(bool active);
    void setArmed(bool armed);
    void setHits(int hits);
    /// 瞄准阶段在次数标签上显示"已选 n / m"，target <= 0 时恢复"扫射 n 次"。
    void setProgress(int selected, int target);
    void clearHits();
    void clearInput();

    /// 输入框里的"扫射次数"（对应规则里的一次覆盖 5 格，非法输入按 0 处理）。
    int requestedVolleys() const;

signals:
    void armRequested();
    void fireRequested();
    void cancelRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateButtons();
    void restoreCredits();

    Side m_side;
    QLabel* m_title = nullptr;
    QLabel* m_creditsLabel = nullptr;
    QLineEdit* m_input = nullptr;
    ThemeButton* m_arm = nullptr;
    ThemeButton* m_fire = nullptr;
    ThemeButton* m_cancel = nullptr;
    QLabel* m_hit = nullptr;

    int m_credits = 0;
    bool m_active = false;
    bool m_armed = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_TURNBAR_H
