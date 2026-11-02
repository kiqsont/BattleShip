#ifndef BS_UI_RULESVIEW_H
#define BS_UI_RULESVIEW_H

#include <QWidget>

namespace Ui {
class RulesView;
}

namespace bs {
namespace ui {

/// 规则说明（替代旧版 RuleScene）。内容与版式沿用原有 .ui，只补齐生命周期与命名。
class RulesView : public QWidget
{
    Q_OBJECT

public:
    explicit RulesView(QWidget* parent = nullptr);
    ~RulesView() override;

signals:
    void backRequested();

private:
    void showPage(int page);

    Ui::RulesView* ui;
    int m_page = 0;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_RULESVIEW_H
