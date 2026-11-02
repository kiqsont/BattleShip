#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QWidget>
#include "label.h"

namespace Ui {
class Configuration;
}

class Configuration : public QWidget
{
    Q_OBJECT

public:
    explicit Configuration(QString);
    ~Configuration();
    label * seaField[10][10];
    int seaArray[10][10];
    int Bship = 1;
    int Cship = 2;
    int Dship = 4;
    int Sship = 2;
    int tranformNum(QString);
    QString tranformStr(int);
    bool handleDirection(int x,int y,int type,int d);
    bool SetShipShang(int x,int y,int type);
    bool SetShipXia(int x,int y,int type);
    bool SetShipZuo(int x,int y,int type);
    bool SetShipYou(int x,int y,int type);
    void DeleteShip(int x,int y,int type,int direction);
    void DeleteS(int x,int y,int type,QPixmap pix);
    void DeleteX(int x,int y,int type,QPixmap pix);
    void DeleteZ(int x,int y,int type,QPixmap pix);
    void DeleteY(int x,int y,int type,QPixmap pix);
signals:
    void MainWindowBack();
    void ConfigFinsh();

private:
    Ui::Configuration *ui;
};

#endif // CONFIGURATION_H
