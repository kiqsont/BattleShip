#include "ui/RulesView.h"

#include "ui_rules.h"

#include <QLabel>
#include <QPixmap>
#include <QPushButton>

namespace bs {
namespace ui {
namespace {

QPixmap loadScaled(const char* path, qreal factor)
{
    QPixmap pix;
    pix.load(QString::fromLatin1(path));
    if (pix.isNull())
        return pix;
    return pix.scaled(static_cast<int>(pix.width() * factor), static_cast<int>(pix.height() * factor),
                      Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace

RulesView::RulesView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::RulesView)
{
    ui->setupUi(this);
    setFixedSize(460, 740);
    setWindowTitle(QStringLiteral("Instruction"));

    ui->label1->setPixmap(QPixmap(QStringLiteral(":/Image/RuleInstruction2.png")).scaled(420, 210));
    ui->label2->setPixmap(loadScaled(":/Image/RuleInstruction1.png", 0.6));
    ui->label3->setPixmap(loadScaled(":/Image/RuleInstruction3.png", 0.6));
    ui->label4->setPixmap(loadScaled(":/Image/RuleInstruction4.png", 0.7));
    ui->label5->setPixmap(loadScaled(":/Image/RuleInstruction5.png", 0.8));
    ui->label6->setPixmap(loadScaled(":/Image/RuleInstruction6.png", 0.3));
    ui->label7->setPixmap(loadScaled(":/Image/RuleInstruction7.png", 0.3));

    connect(ui->backMainButton, &QPushButton::clicked, this, &RulesView::backRequested);
    connect(ui->lastButton, &QPushButton::clicked, this, [this] { showPage(m_page - 1); });
    connect(ui->nextButton, &QPushButton::clicked, this, [this] { showPage(m_page + 1); });

    showPage(0);
}

RulesView::~RulesView()
{
    delete ui;
}

void RulesView::showPage(int page)
{
    const int count = ui->stackedWidget->count();
    if (page < 0 || page >= count)
        return;
    m_page = page;
    ui->stackedWidget->setCurrentIndex(m_page);
}

} // namespace ui
} // namespace bs
