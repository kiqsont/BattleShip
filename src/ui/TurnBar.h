#ifndef BS_UI_TURNBAR_H
#define BS_UI_TURNBAR_H

#include "core/Types.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace bs {
namespace ui {

/// 一侧玩家的行动条：扫射预算、瞄准格数输入、瞄准 / 启动 / 取消、命中反馈。
/// 旧版把这一整套控件（含重叠摆放的 hit 标签）在 Game 构造函数里手工建了两遍。
class TurnBar : public QWidget
{
    Q_OBJECT

public:
    explicit TurnBar(Side side, QWidget* parent = nullptr);

    void setBudget(int budget);
    void setActive(bool active);
    void setArmed(bool armed);
    void setHits(int hits);
    void clearHits();
    void clearInput();

    /// 输入框里的数值（对应旧版的"格数单位"，非法输入按 0 处理）。
    int requestedUnits() const;

signals:
    void armRequested();
    void fireRequested();
    void cancelRequested();

private:
    void updateButtons();

    Side m_side;
    QLabel* m_budget = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_arm = nullptr;
    QPushButton* m_fire = nullptr;
    QPushButton* m_cancel = nullptr;
    QLabel* m_hit = nullptr;

    int m_budgetValue = -1;
    bool m_active = false;
    bool m_armed = false;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_TURNBAR_H
