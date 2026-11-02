#ifndef LABEL_H
#define LABEL_H

#include <QLabel>

class label : public QLabel
{
    Q_OBJECT
public:
    explicit label();
    label(int full);
    void SetShip(QString);
    void SetShip(QPixmap);
signals:
    void fullcheck();
};

#endif // LABEL_H
