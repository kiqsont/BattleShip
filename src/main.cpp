#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QFont>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("BattleShip"));

    // 统一字族，其余页面沿用系统给的默认字号，避免各处字形不一。
    QFont base = QApplication::font();
    base.setFamily(bs::ui::theme::sansFamily());
    QApplication::setFont(base);

    bs::ui::MainWindow window;
    window.show();

    return app.exec();
}
