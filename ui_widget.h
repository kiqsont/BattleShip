/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QPushButton *StartButton;
    QPushButton *Player1;
    QPushButton *Player2;
    QPushButton *Instruction;
    QPushButton *ExitButton;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName(QString::fromUtf8("Widget"));
        Widget->resize(460, 740);
        StartButton = new QPushButton(Widget);
        StartButton->setObjectName(QString::fromUtf8("StartButton"));
        StartButton->setGeometry(QRect(180, 400, 110, 35));
        StartButton->setMinimumSize(QSize(110, 35));
        StartButton->setMaximumSize(QSize(110, 35));
        Player1 = new QPushButton(Widget);
        Player1->setObjectName(QString::fromUtf8("Player1"));
        Player1->setGeometry(QRect(190, 460, 93, 28));
        Player2 = new QPushButton(Widget);
        Player2->setObjectName(QString::fromUtf8("Player2"));
        Player2->setGeometry(QRect(190, 500, 93, 28));
        Instruction = new QPushButton(Widget);
        Instruction->setObjectName(QString::fromUtf8("Instruction"));
        Instruction->setGeometry(QRect(180, 560, 110, 35));
        Instruction->setMinimumSize(QSize(110, 35));
        Instruction->setMaximumSize(QSize(110, 35));
        ExitButton = new QPushButton(Widget);
        ExitButton->setObjectName(QString::fromUtf8("ExitButton"));
        ExitButton->setGeometry(QRect(180, 610, 110, 35));
        ExitButton->setMinimumSize(QSize(110, 35));
        ExitButton->setMaximumSize(QSize(110, 35));

        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "Widget", nullptr));
        StartButton->setText(QCoreApplication::translate("Widget", "\345\274\200\345\247\213", nullptr));
        Player1->setText(QCoreApplication::translate("Widget", "Player1", nullptr));
        Player2->setText(QCoreApplication::translate("Widget", "Player2", nullptr));
        Instruction->setText(QCoreApplication::translate("Widget", "\350\247\204\345\210\231\350\257\264\346\230\216", nullptr));
        ExitButton->setText(QCoreApplication::translate("Widget", "\351\200\200\345\207\272", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
