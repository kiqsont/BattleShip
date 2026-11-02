#include "rulescene.h"
#include "ui_rulescene.h"
#include <QPainter>
#include <QLabel>

RuleScene::RuleScene(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RuleScene)
{
    ui->setupUi(this);
    setFixedSize(460,740);
    setWindowTitle("Instruction");

    connect(ui->backMainButton,&QPushButton::clicked,this,[=](){
        emit MainWindowBack();
    });

    //规则图片加载
    QPixmap label;
    label.load(":/Image/RuleInstruction2.png");
    label = label.scaled(420,210);
    ui->label1->setPixmap(label);

    label.load(":/Image/RuleInstruction1.png");
    label = label.scaled(label.width()*0.6,label.height()*0.6);
    ui->label2->setPixmap(label);

    label.load(":/Image/RuleInstruction3.png");
    label = label.scaled(label.width()*0.6,label.height()*0.6);
    ui->label3->setPixmap(label);

    label.load(":/Image/RuleInstruction4.png");
    label = label.scaled(label.width()*0.7,label.height()*0.7);
    ui->label4->setPixmap(label);

    label.load(":/Image/RuleInstruction5.png");
    label = label.scaled(label.width()*0.8,label.height()*0.8);
    ui->label5->setPixmap(label);

    label.load(":/Image/RuleInstruction6.png");
    label = label.scaled(label.width()*0.3,label.height()*0.3);
    ui->label6->setPixmap(label);

    label.load(":/Image/RuleInstruction7.png");
    label = label.scaled(label.width()*0.3,label.height()*0.3);
    ui->label7->setPixmap(label);

    ui->stackedWidget->setCurrentIndex(0);
    connect(ui->lastButton,&QPushButton::clicked,[=](){
        if(this->page > 0)
        {
            ui->stackedWidget->setCurrentIndex(--page);
        }
    });

    connect(ui->nextButton,&QPushButton::clicked,[=](){
        if(this->page < 2)
        {
            ui->stackedWidget->setCurrentIndex(++page);
        }
    });

}

RuleScene::~RuleScene()
{
    delete ui;
}


