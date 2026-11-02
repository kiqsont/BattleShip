#ifndef BUTTON_H
#define BUTTON_H

#include <QPushButton>

class button : public QPushButton
{
    Q_OBJECT
public:
    explicit button();
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent * e);
    QPixmap pixmap;

    int index;
    bool Close;
    bool Turn;
    bool flag();
    bool Shoot;
    bool ShootTime;
    bool full;
    bool shoot();

    void shootPix();
signals:
    void beShoot();
    void calShoot();
};

#endif // BUTTON_H
