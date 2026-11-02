#ifndef BS_UI_TURNBAR_H
#define BS_UI_TURNBAR_H

#include "core/Types.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace bs {
namespace ui {

/// 一侧玩家的行动条：剩余扫射次数、本次使用次数、瞄准 / 启动 / 取消、命中反馈。
/// 旧版把这一整套控件（含重叠摆放的 hit 标签）在 Game 构造函数里手工建了两遍。
class TurnBar : public QWidget
{
    Q_OBJECT

public:
    explicit TurnBar(Side side, QWidget* parent = nullptr);

    void setVolleyCredits(int credits);
    void setActive(bool active);
    void setArmed(bool armed);
    void setHits(int hits);
    void clearHits();
    void clearInput();

    /// 输入框里的"扫射次数"（对应规则里的一次覆盖 5 格，非法输入按 0 处理）。
    int requestedVolleys() const;

signals:
    void armRequested();
    void fireRequested();
    void cancelRequested();

private:
    void updateButtons();

    Side m_side;
    QLabel* m_creditsLabel = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_arm = nullptr;
    QPushButton* m_fire = nullptr;
    QPushButton* m_cancel = nullptr;
    QLabel* m_hit = nullptr;

    int m_credits = 0;
    bool m_active = false;
    bool m_armed = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_TURNBAR_H
