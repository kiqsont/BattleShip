#ifndef RULESCENE_H
#define RULESCENE_H

#include <QWidget>

namespace Ui {
class RuleScene;
}

class RuleScene : public QWidget
{
    Q_OBJECT

public:
    explicit RuleScene(QWidget *parent = nullptr);
    ~RuleScene();
    int page = 0;

signals:
    void MainWindowBack();

private:
    Ui::RuleScene *ui;


};

#endif // RULESCENE_H
