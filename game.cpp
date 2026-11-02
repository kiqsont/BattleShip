#include "game.h"
#include <QPainter>
#include <QLabel>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit>

Game::Game(QWidget *parent) : QWidget(parent)
{
    //数据初始化
    for(int i = 0;i < 10;i++)
        for(int j = 0;j < 10;j++)
        {
            Player1Array[i][j] = 0;
            Player2Array[i][j] = 0;
        }
    turn = true;
    togeShoot1 = -1;
    togeShoot2 = -1;
    Shoot1 = false;
    Shoot2 = false;
    choic1 = 0;
    choic2 = 0;
    full1 = new label(1);
    full2 = new label(2);
    time1 = 0;
    time2 = 0;



    //标签说明
    setFixedSize(480,900);
    setWindowTitle("Battle Ship");
    QLabel * state1 = new QLabel;
    state1->setFixedSize(140,30);
    state1->setParent(this);
    state1->move(20,5);
    state1->setText("Player1  一同扫射：");
    QLabel * state2 = new QLabel;
    state2->setParent(this);
    state2->setFixedSize(140,30);
    state2->move(20,865);
    state2->setText("Player2  一同扫射：");
    //state2->setText("");

    hit1 = new QLabel;
    hit1->setFixedSize(60,30);
    hit1->setParent(this);
    hit1->move(180,5);
    hit2 = new QLabel;
    hit2->setFixedSize(60,30);
    hit2->setParent(this);
    hit2->move(180,865);


    //一同扫射 次数标签
    times1 = new QLabel;
    times1->setParent(this);
    times1->setFixedSize(30,30);
    times1->move(170,5);
    times1->setNum(0);
    times2 = new QLabel;
    times2->setParent(this);
    times2->setFixedSize(30,30);
    times2->move(170,865);
    times2->setNum(0);


    //一同扫射按钮
    QPushButton * shoot1 = new QPushButton("启动");
    shoot1->setFixedSize(50,30);
    shoot1->setParent(this);
    shoot1->move(335,5);
    QPushButton * shoot2 = new QPushButton("启动");
    shoot2->setFixedSize(50,30);
    shoot2->setParent(this);
    shoot2->move(335,865);

    QPushButton * ShootCancel1 = new QPushButton("取消");
    ShootCancel1->setParent(this);
    ShootCancel1->setFixedSize(50,30);
    ShootCancel1->move(390,5);
    QPushButton * ShootCancel2 = new QPushButton("取消");
    ShootCancel2->setParent(this);
    ShootCancel2->setFixedSize(50,30);
    ShootCancel2->move(390,865);

    QPushButton * aim1 = new QPushButton("瞄准");
    aim1->setFixedSize(50,30);
    aim1->setParent(this);
    aim1->move(280,5);
    QPushButton * aim2 = new QPushButton("瞄准");
    aim2->setFixedSize(50,30);
    aim2->setParent(this);
    aim2->move(280,865);

    QLineEdit * line1 = new QLineEdit;
    line1->setFixedSize(18,21);
    line1->setParent(this);
    line1->move(257,8);

    QLineEdit * line2 = new QLineEdit;
    line2->setFixedSize(18,21);
    line2->setParent(this);
    line2->move(257,872);


    //瞄准1
    connect(aim1,&QPushButton::clicked,[=](){
        hit1->clear();
        if(togeShoot1 > 0)
        {
            Shoot1 = true;
            choic1 = 0;
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton1[i][j]->Shoot)
                    {
                        seaButton1[i][j]->Shoot = false;
                        seaButton1[i][j]->shootPix();
                    }
                    seaButton1[i][j]->ShootTime = false;
                    seaButton1[i][j]->full =false;
                }
            //取消之前所选的位置
            time1 = line1->displayText().toInt() * 5;
            if(time1 > togeShoot1 * 5)
                time1 = togeShoot1 * 5;
            choic1 = time1;
            Shoot(1);
        }
    });

    //取消瞄准
    connect(ShootCancel1,&QPushButton::clicked,[=](){
        choic1 = 0;
        Shoot1 = false;
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                if(seaButton1[i][j]->Shoot)
                {
                    seaButton1[i][j]->Shoot = false;
                    seaButton1[i][j]->shootPix();
                }
                seaButton1[i][j]->ShootTime = false;
                seaButton1[i][j]->full =false;
            }
        line1->clear();
    });

    //发射导弹
    connect(shoot1,&QPushButton::clicked,[=](){
        if(Shoot1)
        {
            //qDebug() << choic/5;
            togeShoot1 -= time1/5;
            int num = 0;
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton1[i][j]->Shoot)
                    {
                        if(seaButton1[i][j]->shoot())
                        {
                            num++;
                        }
                    }
                }
            times1->setNum(togeShoot1);
            ShootCancel1->click();
            QString str = "击中：" + QString::number(num);
            hit1->setText(str);
            WinCheck(1);
            if(num == 0)
            {
                togeShoot1 = -1;
                turn = false;
                Rotation();
            }
        }
    });


    //瞄准2
    connect(aim2,&QPushButton::clicked,[=](){
        hit2->clear();
        if(togeShoot2 > 0)
        {
            Shoot2 = true;
            choic2 = 0;
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton2[i][j]->Shoot)
                    {
                        seaButton2[i][j]->Shoot = false;
                        seaButton2[i][j]->shootPix();
                    }
                    seaButton2[i][j]->ShootTime = false;
                    seaButton2[i][j]->full =false;
                }
            //取消之前所选的位置
            time2 = line2->displayText().toInt() * 5;
            if(time2 > togeShoot2 * 5)
                time2 = togeShoot2 * 5;
            choic2 = time2;
            Shoot(2);
        }
    });

    //取消瞄准
    connect(ShootCancel2,&QPushButton::clicked,[=](){
        choic2 = 0;
        Shoot2 = false;
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                if(seaButton2[i][j]->Shoot)
                {
                    seaButton2[i][j]->Shoot = false;
                    seaButton2[i][j]->shootPix();
                }
                seaButton2[i][j]->ShootTime = false;
                seaButton2[i][j]->full =false;
            }
        line2->clear();
    });

    //发射导弹2
    connect(shoot2,&QPushButton::clicked,[=](){
        if(Shoot2)
        {
            //qDebug() << choic/5;
            togeShoot2 -= time2/5;
            int num = 0;
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton2[i][j]->Shoot)
                    {
                        if(seaButton2[i][j]->shoot())
                        {
                            num++;
                        }
                    }
                }
            times2->setNum(togeShoot1);
            ShootCancel2->click();
            QString str = "击中：" + QString::number(num);
            hit2->setText(str);
            WinCheck(2);
            if(num == 0)
            {
                togeShoot2 = -1;
                turn = true;
                Rotation();
            }
        }
    });


}

void Game::install()
{
    for(int i = 0;i < 10;i++)
    {
        for(int j = 0;j < 10;j++)
        {
            //玩家1
            button * btn = new button;
            seaButton1[i][j] = btn;
            btn->setParent(this);
            btn->move(40+j*40,40+i*40);
            btn->index = Player1Array[i][j];

            connect(btn,&button::beShoot,[=](){
                choic1--;
                emit full1->fullcheck();
               //qDebug() << "hear add test";
            });
            connect(btn,&button::calShoot,[=](){
                choic1++;
                emit full1->fullcheck();
                //qDebug() << "hear cancel test";
            });

            connect(btn,&QPushButton::clicked,[=](){
                //qDebug() << btn->index;
                hit1->clear();
                //点击了一个
                if(btn->flag())
                {
                    togeShoot1++;
                    WinCheck(1);
                    if(togeShoot1 > 0)
                    {
                        times1->setNum(togeShoot1);
                    }
                }
                else
                {
                    turn = false;
                    togeShoot1 = -1;
                    times1->setNum(0);
                }

                Rotation();
            });




            //玩家2
            button * btn2 = new button;
            seaButton2[i][j] = btn2;
            btn2->setParent(this);
            btn2->move(40+j*40,460+i*40);
            btn2->index = Player2Array[i][j];
            btn2->Turn = false;

            connect(btn,&button::beShoot,[=](){
                choic2--;
                emit full2->fullcheck();
               //qDebug() << "hear add test";
            });
            connect(btn,&button::calShoot,[=](){
                choic2++;
                emit full2->fullcheck();
                //qDebug() << "hear cancel test";
            });

            connect(btn2,&QPushButton::clicked,[=](){
                //qDebug() << btn2->index;
                hit2->clear();
                if(btn2->flag())
                {
                    togeShoot2++;
                    WinCheck(2);
                    if(togeShoot2 > 0)
                    {
                        times2->setNum(togeShoot2);
                    }
                }
                else
                {
                    turn = true;
                    togeShoot2 = -1;
                    times2->setNum(0);
                }
                Rotation();

            });
        }
    }


    //监听一同扫射时是否满足times个
    connect(full1,&label::fullcheck,[=](){
        //当满了的时候
       // qDebug() << "hear fullcheck";
        //qDebug() << choic1;
        if(choic1 == 0)
        {
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton1[i][j]->Shoot != true)
                    {
                        seaButton1[i][j]->full = true;
                        //qDebug() << "full";
                    }
                }
        }
        else
        {
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                    seaButton1[i][j]->full = false;
        }
    });

    //监听一同扫射时是否满足times个
    connect(full2,&label::fullcheck,[=](){
        //当满了的时候
       // qDebug() << "hear fullcheck";
        //qDebug() << choic2;
        if(choic2 == 0)
        {
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                {
                    if(seaButton2[i][j]->Shoot != true)
                    {
                        seaButton2[i][j]->full = true;
                        //qDebug() << "full";
                    }
                }
        }
        else
        {
            for(int i = 0;i < 10;i++)
                for(int j = 0;j < 10;j++)
                    seaButton2[i][j]->full = false;
        }
    });




}

void Game::WinCheck(int i)
{
    if(i == 1)
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                if(seaButton1[i][j]->index != 0)
                    return;
            }
        gameEnd(1);
    }
    if(i == 2)
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                if(seaButton2[i][j]->index != 0)
                    return;
            }
        gameEnd(2);
    }
}

void Game::Rotation()
{
    if(turn)
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                seaButton1[i][j]->Turn = true;
                seaButton2[i][j]->Turn = false;
            }
    }
    else
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
            {
                seaButton2[i][j]->Turn = true;
                seaButton1[i][j]->Turn = false;
            }
    }
}

void Game::gameEnd(int num)
{
    QString str = "恭喜玩家" + QString::number(num) + "取得胜利";
    if(QMessageBox::Close == QMessageBox::information(this ,"Congratulations",str,QMessageBox::Close))
    {
        this->hide();
        emit winBack();
    }
}

void Game::Shoot(int id)
{
    if(id == 1)
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
                seaButton1[i][j]->ShootTime = true;

    }
    if(id == 2)
    {
        for(int i = 0;i < 10;i++)
            for(int j = 0;j < 10;j++)
                seaButton2[i][j]->ShootTime = true;

    }


}

void Game::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPen pen;
    pen.setWidth(5);
    pen.setStyle(Qt::DashDotLine);
    painter.setPen(pen);
    painter.drawLine(QPoint(0,450),QPoint(480,450));
}
