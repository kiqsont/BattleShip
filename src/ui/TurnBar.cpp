#include "ui/TurnBar.h"

#include "core/Rules.h"
#include "ui/GameControls.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QPen>

namespace bs {
namespace ui {

TurnBar::TurnBar(Side side, QWidget* parent)
    : QWidget(parent)
    , m_side(side)
{
    // 单行紧凑版：高 36px，直接放在对战页首尾，不插进两块棋盘中间。
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 3, 8, 3);
    layout->setSpacing(5);

    m_title = new QLabel(sideTitle(m_side), this);
    styleLabel(m_title, 13, true);
    layout->addWidget(m_title);

    m_creditsLabel = new QLabel(QStringLiteral("扫射 0 次"), this);
    m_creditsLabel->setToolTip(
        QStringLiteral("已累积的扫射次数（每击沉一艘舰得一次）；瞄准时显示已选格数"));
    styleLabel(m_creditsLabel, 12);
    layout->addWidget(m_creditsLabel);

    m_input = new QLineEdit(this);
    m_input->setFixedWidth(34);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setPlaceholderText(QStringLiteral("次数"));
    m_input->setToolTip(QStringLiteral("本次使用几次扫射，每次覆盖 %1 格").arg(rules::kCellsPerVolley));
    styleLineEdit(m_input);
    layout->addWidget(m_input);

    m_arm = new ThemeButton(QStringLiteral("瞄准"), this);
    m_arm->setFixedSize(42, 24);
    layout->addWidget(m_arm);

    m_fire = new ThemeButton(QStringLiteral("开火"), this, ThemeButton::Accent::Seal);
    m_fire->setFixedSize(42, 24);
    layout->addWidget(m_fire);

    m_cancel = new ThemeButton(QStringLiteral("取消"), this);
    m_cancel->setFixedSize(42, 24);
    layout->addWidget(m_cancel);

    m_hit = new QLabel(this);
    styleLabel(m_hit, 12, true);
    layout->addWidget(m_hit);

    layout->addStretch(1);

    connect(m_arm, &ThemeButton::clicked, this, &TurnBar::armRequested);
    connect(m_fire, &ThemeButton::clicked, this, &TurnBar::fireRequested);
    connect(m_cancel, &ThemeButton::clicked, this, &TurnBar::cancelRequested);

    // 高度交给外层布局按 stretch 比例分配，不再钉死 36px。
    setMinimumHeight(26);
    updateButtons();
}

void TurnBar::paintEvent(QPaintEvent*)
{
    // 极淡的纸面卡片 + 发丝线，不再画醒目的朱红"选中框"。
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF box(rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme::paper());
    painter.drawRoundedRect(box, 4, 4);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(theme::hairline(), 1));
    painter.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
}

void TurnBar::restoreCredits()
{
    m_creditsLabel->setText(QStringLiteral("扫射 %1 次").arg(m_credits > 0 ? m_credits : 0));
}

void TurnBar::setVolleyCredits(int credits)
{
    m_credits = credits;
    restoreCredits();
    updateButtons();
}

void TurnBar::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    m_input->setEnabled(active);
    // 当前行动方只用标题颜色点一下，不再给整条画红框。
    QPalette palette = m_title->palette();
    palette.setColor(QPalette::WindowText, active ? theme::sealRed() : theme::ink());
    m_title->setPalette(palette);
    updateButtons();
}

void TurnBar::setArmed(bool armed)
{
    m_armed = armed;
    if (armed)
        clearHits();
    updateButtons();
}

void TurnBar::setHits(int hits)
{
    m_hit->setText(QStringLiteral("击中 %1").arg(hits));
}

void TurnBar::setProgress(int selected, int target)
{
    if (target <= 0) {
        restoreCredits();
        return;
    }
    m_creditsLabel->setText(QStringLiteral("已选 %1 / %2").arg(selected).arg(target));
}

void TurnBar::clearHits()
{
    m_hit->clear();
}

void TurnBar::clearInput()
{
    m_input->clear();
}

int TurnBar::requestedVolleys() const
{
    return m_input->text().toInt();
}

void TurnBar::updateButtons()
{
    m_arm->setEnabled(m_active && !m_armed && m_credits > 0);
    m_fire->setEnabled(m_active && m_armed);
    m_cancel->setEnabled(m_active && m_armed);
}

} // namespace ui
} // namespace bs
