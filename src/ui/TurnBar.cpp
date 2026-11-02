#include "ui/TurnBar.h"

#include "core/Rules.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace bs {
namespace ui {

TurnBar::TurnBar(Side side, QWidget* parent)
    : QWidget(parent)
    , m_side(side)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(6);

    auto* title = new QLabel(sideTitle(m_side) + QStringLiteral("  一同扫射："), this);
    layout->addWidget(title);

    m_budget = new QLabel(QStringLiteral("0"), this);
    m_budget->setFixedWidth(28);
    m_budget->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_budget);

    m_input = new QLineEdit(this);
    m_input->setFixedWidth(30);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setPlaceholderText(QStringLiteral("1"));
    m_input->setToolTip(QStringLiteral("1 个单位 = %1 格扫射预算").arg(rules::kCellsPerVolleyUnit));
    layout->addWidget(m_input);

    m_arm = new QPushButton(QStringLiteral("瞄准"), this);
    m_arm->setFixedWidth(50);
    layout->addWidget(m_arm);

    m_fire = new QPushButton(QStringLiteral("启动"), this);
    m_fire->setFixedWidth(50);
    layout->addWidget(m_fire);

    m_cancel = new QPushButton(QStringLiteral("取消"), this);
    m_cancel->setFixedWidth(50);
    layout->addWidget(m_cancel);

    m_hit = new QLabel(this);
    m_hit->setMinimumWidth(70);
    layout->addWidget(m_hit, 1);

    connect(m_arm, &QPushButton::clicked, this, &TurnBar::armRequested);
    connect(m_fire, &QPushButton::clicked, this, &TurnBar::fireRequested);
    connect(m_cancel, &QPushButton::clicked, this, &TurnBar::cancelRequested);

    setFixedHeight(36);
    updateButtons();
}

void TurnBar::setBudget(int budget)
{
    m_budgetValue = budget;
    m_budget->setNum(budget > 0 ? budget : 0);
    updateButtons();
}

void TurnBar::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    m_input->setEnabled(active);
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
    m_hit->setText(QStringLiteral("击中：%1").arg(hits));
}

void TurnBar::clearHits()
{
    m_hit->clear();
}

void TurnBar::clearInput()
{
    m_input->clear();
}

int TurnBar::requestedUnits() const
{
    return m_input->text().toInt();
}

void TurnBar::updateButtons()
{
    m_arm->setEnabled(m_active && !m_armed && m_budgetValue > 0);
    m_fire->setEnabled(m_active && m_armed);
    m_cancel->setEnabled(m_active && m_armed);
}

} // namespace ui
} // namespace bs
