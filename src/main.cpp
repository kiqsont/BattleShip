#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("BattleShip"));

    bs::ui::MainWindow window;
    window.show();

    return app.exec();
}
