#ifndef BS_UI_RULESVIEW_H
#define BS_UI_RULESVIEW_H

#include <QWidget>

class QHBoxLayout;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace bs {
namespace ui {

/// 规则说明页。
///
/// 旧版是 460×740 的 .ui：七个 QLabel 绝对定位贴图 + 三个写着 Page:1/2/3 的假页码。
/// 现在是一张可滚动的纸面排版：页眉 + 大字号章节 + 插图 + 图例。
/// 文字栏限制最大宽度并按窗口居中，窗口缩放时文字与插图一起重排。
class RulesView : public QWidget
{
    Q_OBJECT

public:
    explicit RulesView(QWidget* parent = nullptr);

signals:
    void backRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildHeader();
    QWidget* buildContent();
    void updateColumnWidth();

    QPushButton* m_back = nullptr;
    QScrollArea* m_scroll = nullptr;
    QHBoxLayout* m_headerLayout = nullptr;
    QVBoxLayout* m_columnLayout = nullptr;
};

} // namespace ui
} // namespace bs

#endif // BS_UI_RULESVIEW_H
