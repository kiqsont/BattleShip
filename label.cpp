#include "label.h"

label::label()
{
    setFixedSize(40,40);
    QPixmap pix(":/Image/BasicSquare.png");
    pix = pix.scaled(pix.width()*0.4,pix.height()*0.4);
    setPixmap(pix);
}

label::label(int full)
{
    Q_UNUSED(full);
}

void label::SetShip(QString res)
{
    QPixmap pixmap(res);
    pixmap = pixmap.scaled(40,40);
    setPixmap(pixmap);
}

void label::SetShip(QPixmap pix)
{
    pix = pix.scaled(40,40);
    setPixmap(pix);
}

