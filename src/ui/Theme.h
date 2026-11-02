#ifndef BS_UI_THEME_H
#define BS_UI_THEME_H

#include <QColor>
#include <QFont>

class QPainter;
class QRect;

namespace bs {
namespace ui {

/// 全站统一的视觉语言。
///
/// 素材是黑白漫画（StartBackground / RuleInstruction*），所以只保留三种颜色：
/// 墨（近黑）、纸（暖白）、朱红（点到为止）。层次靠字号、字重、留白和 1px 细线，
/// 不靠阴影与粗边框。颜色与字体只在这里定义，页面里不再出现硬编码色值。
namespace theme {

QColor ink();       ///< 主墨色，文字与深色面板
QColor inkSoft();   ///< 正文
QColor muted();     ///< 辅助说明
QColor paper();     ///< 纸面底色
QColor paperDeep(); ///< 更浅的灰，用于大号章节数字
QColor sealRed();   ///< 朱红，仅用于极少量点缀
QColor hairline();  ///< 发丝线

QString sansFamily();
QString monoFamily();

QFont displayFont(int pixelSize); ///< 大标题：加粗、字距放宽
QFont titleFont(int pixelSize);   ///< 小节标题
QFont bodyFont(int pixelSize, int weight = QFont::Normal); ///< 正文，可指定字重
QFont capsFont(int pixelSize);    ///< 全大写英文小字，字距最宽
QFont monoFont(int pixelSize);    ///< 编号

/// 纸面：暖白底 + 自上而下极轻的渐变，不带边框。
void paintPaper(QPainter& painter, const QRect& rect);

/// 带极细描边的文字。
///
/// 中文在 14~17px 下笔画本来就比拉丁字母细，单纯 setBold 仍然发虚；
/// 补一圈描边可以让字"实"起来，又不会像整段加粗那样糊。
void drawHeavyText(QPainter& painter, const QRect& rect, const QString& text, const QFont& font,
                   const QColor& color, qreal strokeWidth = 0.6,
                   Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter);

} // namespace theme
} // namespace ui
} // namespace bs

#endif // BS_UI_THEME_H
