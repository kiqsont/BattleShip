#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include "rulescene.h"
#include "configuration.h"
#include "game.h"

#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setFixedSize(460,740);
    setWindowTitle("Battle Ship");

    //Exit按钮
    connect(ui->ExitButton,&QPushButton::clicked,[=](){
        this->close();
    });


    //规则说明界面
    RuleScene * instruction = new RuleScene;
    connect(ui->Instruction,&QPushButton::clicked,[=](){
        instruction->setGeometry(this->geometry());
        this->hide();
        instruction->show();
        qDebug() << "show rule scene";
    });
    connect(instruction,&RuleScene::MainWindowBack,this,[=](){
        this->setGeometry(instruction->geometry());
        instruction->hide();
        this->show();
    });



    //海域信息初始化
    for(int i = 0;i < 10;i++)
        for(int j = 0;j < 10;j++)
        {
            Player1Array[i][j] = 0;
            Player2Array[i][j] = 0;
        }

    //开始
    Game * game = new Game;
    connect(ui->StartButton,&QPushButton::clicked,[=](){
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                game->Player1Array[i][j] = Player1Array[i][j];
                game->Player2Array[i][j] = Player2Array[i][j];
            }
        game->install();
        game->setGeometry(this->geometry());
        this->hide();
        game->show();
    });

    connect(game,&Game::winBack,this,[=](){
        this->show();
    });


    //玩家1
    Configuration * player1 = new Configuration("Player 1");
    connect(ui->Player1,&QPushButton::clicked,[=](){
        player1->setGeometry(this->geometry());
        this->hide();
        player1->show();
    });
    //返回
    connect(player1,&Configuration::MainWindowBack,[=](){
        this->setGeometry(player1->geometry());
        player1->hide();
        this->show();
    });
    //记录数据
    connect(player1,&Configuration::ConfigFinsh,[=](){
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                Player1Array[i][j] = player1->seaArray[i][j];
            }
        this->show();
        player1->hide();
    });

    //玩家2
    Configuration * player2 = new Configuration("Player 2");
    connect(ui->Player2,&QPushButton::clicked,[=](){
        player2->setGeometry(this->geometry());
        this->hide();
        player2->show();
    });
    //返回
    connect(player2,&Configuration::MainWindowBack,[=](){
        this->setGeometry(player2->geometry());
        player2->hide();
        this->show();
    });
    //记录数据
    connect(player2,&Configuration::ConfigFinsh,[=](){
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                Player2Array[i][j] = player2->seaArray[i][j];
            }
        this->show();
        player2->hide();
    });


}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *)
{
    QPixmap bg;
    bg.load(":/Image/StartBackground.png");
    QPainter painter(this);
    painter.drawPixmap(0,0,width(),height(),bg);
}

