#ifndef GAME_H
#define GAME_H

#include <QWidget>
#include "button.h"
#include <QLabel>
#include "label.h"

class Game : public QWidget
{
    Q_OBJECT
public:
    explicit Game(QWidget *parent = nullptr);
    int Player1Array[10][10];
    int Player2Array[10][10];
    void paintEvent(QPaintEvent *event) override;
    button * seaButton1[10][10];
    button * seaButton2[10][10];
    void install();
    void WinCheck(int);
    bool turn;//1为Player1，0为Player2
    void Rotation();
    void gameEnd(int num);
    int togeShoot1;
    int togeShoot2;
    QLabel * times1;
    QLabel * times2;
    label * full1;
    label * full2;
    bool Shoot1;
    bool Shoot2;
    void Shoot(int id);
    int choic1;
    int choic2;
    QLabel * hit1;
    QLabel * hit2;
    int time1;
    int time2;
signals:
    //void choicChange();
    void winBack();
};

#endif // GAME_H
