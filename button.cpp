#include "button.h"
#include <QDebug>

button::button()
{
    Shoot = false;
    full = false;
    ShootTime = false;
    Turn = true;
    Close = true;
    pixmap.load(":/Image/BasicSquare.png");
    pixmap = pixmap.scaled(40,40);
    setFixedSize(pixmap.width(),pixmap.height());
    //setStyleSheet("QPushButton{border:0px;}");
    setIcon(pixmap);
    setIconSize(QSize(pixmap.width(),pixmap.height()));

}

void button::enterEvent(QEvent *)
{
    if(Close && Turn && !Shoot)
    {
        pixmap.load(":/Image/trace.png");
        pixmap = pixmap.scaled(40,40);
        setIcon(pixmap);
    }
}

void button::leaveEvent(QEvent *)
{
    if(Close && Turn && !Shoot)
    {
        pixmap.load(":/Image/BasicSquare.png");
        pixmap = pixmap.scaled(40,40);
        setIcon(pixmap);
    }
}

bool button::flag()
{
        if(index == 0)
        {
            pixmap.load(":/Image/Miss.png");
            pixmap = pixmap.scaled(40,40);
            setIcon(pixmap);
            Close = false;
            return false;
        }
        else
        {
    //        pixmap.load(":/Image/Cruiser.png");
    //        pixmap = pixmap.scaled(40,40);
    //        setIcon(pixmap);
            int type = index % 4;
            switch (type)
            {
                case 0:pixmap.load(":/Image/B.png");break;
                case 3:pixmap.load(":/Image/Cruiser.png");break;
                case 2:pixmap.load(":/Image/D.png");break;
                case 1:pixmap.load(":/Image/S.png");break;
            }

            pixmap = pixmap.scaled(40,40);
            setIcon(pixmap);
            index = 0;
            Close = false;
            return true;
        }
}

bool button::shoot()
{

    pixmap.load(":/Image/togeShoot.png");
    if(index%4==1)
        pixmap.load(":/Image/S.png");
    pixmap = pixmap.scaled(40,40);
    setIcon(pixmap);
    Close = false;
    Shoot = false;
    if(index != 0)
    {
        index = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void button::shootPix()
{
    if(Shoot)
    {
        pixmap.load(":/Image/trace.png");
    }
    else
    {
        pixmap.load(":/Image/BasicSquare.png");
    }
    setIcon(pixmap);
}

void button::mousePressEvent(QMouseEvent * e)
{
    if(!Turn)
    {
        return;
    }

    if(!Close)
    {
        return;
    }

    if(ShootTime)
    {
        if(full)
        {
            return;
        }

        Shoot = !Shoot;
        if(Shoot)
        {

            emit beShoot();
            //qDebug() << "add test";
        }

        else
        {
            emit calShoot();
            //qDebug() << "cancel test";
        }
        shootPix();
        return;
    }

    QPushButton::mousePressEvent(e);
}
