#include "configuration.h"
#include "ui_configuration.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

Configuration::Configuration(QString title) :
    ui(new Ui::Configuration)
{
    ui->setupUi(this);
    setFixedSize(460,740);
    setWindowTitle(title);

    for(int i = 0;i < 10;i++)
        for(int j = 0;j < 10;j++)
            seaArray[i][j] = 0;



    //剩余量显示
    ui->Bsurplus->setText(QString::number(Bship));
    ui->Csurplus->setText(QString::number(Cship));
    ui->Dsurplus->setText(QString::number(Dship));
    ui->Ssurplus->setText(QString::number(Sship));

    //返回按钮
    QPushButton * btn = new QPushButton("返回");
    btn->setParent(this);
    btn->setFixedSize(93,28);
    btn->move(this->width()-btn->width()-20,this->height()-btn->height()-20);
    connect(btn,&QPushButton::clicked,[=](){
        emit MainWindowBack();
    });

    //保存按钮
    QPushButton * save = new QPushButton("保存");
    save->setParent(this);
    save->setFixedSize(93,28);
    save->move(this->width()-btn->width()-20,this->height()-btn->height()-25-save->height());
    connect(save,&QPushButton::clicked,[=](){
        if(Bship == 0 && Cship == 0&& Dship == 0&& Sship == 0)
        {
            //消息对话框 information 是否保存？
            if(QMessageBox::Save == QMessageBox::question(this,"Check","确定保存该配置吗？",QMessageBox::Save|QMessageBox::Cancel))
            {
                emit this->ConfigFinsh();
            }
        }
        else
        {
            if(QMessageBox::Yes == QMessageBox::warning(this,"Back?","没有使用完所有的船只，是否返回主界面？",QMessageBox::Yes|QMessageBox::No))
            {
                emit this->ConfigFinsh();
            }
        }

//        for(int i = 0;i <10;i++)
//        {
//            QString str = "";
//            for(int j = 0;j < 10;j++)
//            {
//                str += QString::number(seaArray[i][j]) + "   ";
//            }
//            qDebug() << str;
//        }
    });



    //  X -> j  Y -> i
    //文字框的X是y（i）  Y是x（j）

    //海域区域index
    for(int i = 0;i < 10;i++)
    {
        QLabel * label = new QLabel;
        label->setParent(this);
        label->setFixedSize(40,40);
        label->move(30+i*40,0);
        label->setText(QString::number(i+1));
        label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    }
    for(int i = 0;i < 10;i++)
    {
        QLabel * label = new QLabel;
        label->setParent(this);
        label->setFixedSize(40,40);
        label->move(0,30+i*40);
        label->setText(QString::number(i+1));
        label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    }

    //海域区域
    for(int i = 0;i < 10;i++)
    {
        for(int j = 0;j < 10;j++)
        {
            label * seafield = new label;
            this->seaField[i][j] = seafield;
            seafield->setGeometry(0,0,40,40);
            seafield->setParent(this);
            seafield->move(30+j*40,30+i*40);
        }
    }

    //增援船舰

    //战舰
    connect(ui->btnB,&QPushButton::clicked,[=](){
        if(this->Bship > 0)
        {
            int x = ui->BlineX->displayText().toInt() - 1;
            int y = ui->BlineY->displayText().toInt() - 1;
            QString dstr = ui->BlineD->displayText();
            int d = tranformNum(dstr);

            if(handleDirection(x,y,4,d))
            {
                ui->Bsurplus->setText(QString::number(--Bship));
                ui->BlineX->clear();
                ui->BlineY->clear();
                ui->BlineD->clear();
//            for(int i = 0;i < 10;i++)
//               for(int j = 0;j < 10 ;j ++)
//                   qDebug() << seaArray[i][j];
            }
        }
        else
        {
            QMessageBox::critical(this,"critical","没有多余的战舰了");
        }
    });

    //巡洋舰
    connect(ui->btnC,&QPushButton::clicked,[=](){
        if(this->Cship > 0)
        {
            int x = ui->ClineX->displayText().toInt() - 1;
            int y = ui->ClineY->displayText().toInt() - 1;
            QString dstr = ui->ClineD->displayText();
            int d = tranformNum(dstr);

            if(handleDirection(x,y,3,d))
            {
                ui->Csurplus->setText(QString::number(--Cship));
                ui->ClineX->clear();
                ui->ClineY->clear();
                ui->ClineD->clear();
            }
        }
        else
        {
            QMessageBox::critical(this,"critical","没有多余的巡洋舰了");
        }
    });

    //驱逐舰
    connect(ui->btnD,&QPushButton::clicked,[=](){
        if(this->Dship > 0)
        {
            int x = ui->DlineX->displayText().toInt() - 1;
            int y = ui->DlineY->displayText().toInt() - 1;
            QString dstr = ui->DlineD->displayText();
            int d = tranformNum(dstr);

            if(handleDirection(x,y,2,d))
            {
                ui->Dsurplus->setText(QString::number(--Dship));
                ui->DlineX->clear();
                ui->DlineY->clear();
                ui->DlineD->clear();
            }
        }
        else
        {
            QMessageBox::critical(this,"critical","没有多余的驱逐舰了");
        }
    });

    //潜水艇
    connect(ui->btnS,&QPushButton::clicked,[=](){
        if(this->Sship > 0)
        {
            int x = ui->SlineX->displayText().toInt() - 1;
            int y = ui->SlineY->displayText().toInt() - 1;
            QString dstr = ui->SlineD->displayText();
            int d = tranformNum(dstr);

            if(handleDirection(x,y,1,d))
            {
                ui->Ssurplus->setText(QString::number(--Sship));
                ui->SlineX->clear();
                ui->SlineY->clear();
                ui->SlineD->clear();
            }

        }
        else
        {
            QMessageBox::critical(this,"critical","没有多余的潜水艇了");
        }
    });








    //删除
    ui->kuang->setAttribute(Qt::WA_TransparentForMouseEvents);
    connect(ui->DELbtn,&QPushButton::clicked,[=](){
        if(ui->delX->text().isEmpty() || ui->delY->text().isEmpty())
            return;
        int y = ui->delX->displayText().toInt()-1;//j
        int x = ui->delY->displayText().toInt()-1;//i
        if(seaArray[x][y] != 0 && seaArray[x][y] != 1 && seaArray[x][y] != 2 && seaArray[x][y] != 3 && seaArray[x][y] != 4 )
        {
            int type = seaArray[x][y] % 4;
            int directon = (seaArray[x][y] - 1) / 4;
            DeleteShip(x,y,type,directon);
        }
        else
        {
            QMessageBox::critical(this,"critical","该位置没有可删除船只");
        }
    });


}

Configuration::~Configuration()
{
    delete ui;
}

int Configuration::tranformNum(QString str)
{
    if(str == "x")
    {
        return 1;
    }
    if(str == "z")
    {
        return 2;
    }
    if(str == "y")
    {
        return 3;
    }
    return 0;
}

QString Configuration::tranformStr(int type)
{
    if(type == 1)
    {
        return "潜水艇";
    }
    if(type == 3)
    {
        return "巡洋舰";
    }
    if(type == 2)
    {
        return "驱逐舰";
    }   

    return "战舰";
}

bool Configuration::handleDirection(int x, int y, int type, int d)
{
    switch(d)
    {
        case 0:return SetShipShang(x,y,type);
        case 1:return SetShipXia(x,y,type);
        case 2:return SetShipZuo(x,y,type);
        case 3:return SetShipYou(x,y,type);
    }
    return false;
}

bool Configuration::SetShipShang(int x, int y, int type)
{
    if(x + type <= 10 && x >= 0&& seaArray[x+type-1][y] == 0)
    {
        if(seaArray[x][y] != 0)
        {
            QMessageBox::critical(this,"critical","与其它船只相撞");
            return false;
        }
        seaArray[x][y] = type+4;
        seaField[x][y]->SetShip(":/Image/ShipHead.png");
        for(int i = 1;i <= type - 1;i++)
        {
            if(seaArray[x+i][y] != 0)
            {
                QMessageBox::critical(this,"critical","与其它船只相撞");
                return false;
            }
            seaField[x+i][y]->SetShip(":/Image/ShipBody.png");
            seaArray[x+i][y] = type;
        }
        return true;
    }
    else
    {
        QString str = "该位置不满足布置" + tranformStr(type);
        str += "的要求";
        QMessageBox::critical(this,"critical",str);
        return false;
    }
}

bool Configuration::SetShipXia(int x, int y, int type)
{
    if(x -type+1 >=0 && x <= 9 && seaArray[x-type+1][y] == 0)
    {
        QMatrix matrix;
        matrix.rotate(180);
        if(seaArray[x][y] != 0)
        {
            QMessageBox::critical(this,"critical","与其它船只相撞");
            return false;
        }
        seaArray[x][y] = type+8;
        seaField[x][y]->SetShip(QPixmap(":/Image/ShipHead.png").transformed(matrix,Qt::SmoothTransformation));
        for(int i = 1;i <= type - 1;i++)
        {
            if(seaArray[x-i][y] != 0)
            {

                QMessageBox::critical(this,"critical","与其它船只相撞");
                return false;
            }
            seaField[x-i][y]->SetShip(":/Image/ShipBody.png");
            seaArray[x-i][y] = type;
        }
        return true;
    }
    else
    {
        QString str = "该位置不满足布置" + tranformStr(type);
        str += "的要求";
        QMessageBox::critical(this,"critical",str);
        return false;
    }
}

bool Configuration::SetShipZuo(int x, int y, int type)
{
    if(y >=0 && y+type <= 10&& seaArray[x][y+type-1] == 0)
    {
        QMatrix matrix;
        matrix.rotate(270);

        if(seaArray[x][y] != 0)
        {
            QMessageBox::critical(this,"critical","与其它船只相撞");
            return false;
        }
        seaArray[x][y] = type + 12;
        seaField[x][y]->SetShip(QPixmap(":/Image/ShipHead.png").transformed(matrix,Qt::SmoothTransformation));
        for(int i = 1;i <= type - 1;i++)
        {
            if(seaArray[x][y+i] != 0)
            {
                QMessageBox::critical(this,"critical","与其它船只相撞");
                return false;
            }
            seaField[x][y+i]->SetShip(":/Image/ShipBody.png");
            seaArray[x][y+i] = type;
        }
        return true;
    }
    else
    {
        QString str = "该位置不满足布置" + tranformStr(type);
        str += "的要求";
        QMessageBox::critical(this,"critical",str);
        return false;
    }
}

bool Configuration::SetShipYou(int x, int y, int type)
{
    if(y <= 9 && y - type + 1 >= 0 && seaArray[x][y-type+1] == 0)
    {
        QMatrix matrix;
        matrix.rotate(90);

        if(seaArray[x][y] != 0)
        {
            QMessageBox::critical(this,"critical","与其它船只相撞");
            return false;
        }
        seaArray[x][y] = type+16;
        seaField[x][y]->SetShip(QPixmap(":/Image/ShipHead.png").transformed(matrix,Qt::SmoothTransformation));
        for(int i = 1;i <= type - 1;i++)
        {
            if(seaArray[x][y-i] != 0)
            {
                QMessageBox::critical(this,"critical","与其它船只相撞");
                return false;
            }
            seaField[x][y-i]->SetShip(":/Image/ShipBody.png");
            seaArray[x][y-i] = type;
        }
        return true;
    }
    else
    {
        QString str = "该位置不满足布置" + tranformStr(type);
        str += "的要求";
        QMessageBox::critical(this,"critical",str);
        return false;
    }
}

void Configuration::DeleteShip(int x, int y, int type,int direction)
{
    seaArray[x][y] = 0;
    QPixmap pix(":/Image/BasicSquare.png");
    pix = pix.scaled(pix.width()*0.4,pix.height()*0.4);
    seaField[x][y]->setPixmap(pix);
    if(type == 1)
    {
        ui->Ssurplus->setText(QString::number(++Sship));
        return;
    }
    if(type == 2)
        ui->Dsurplus->setText(QString::number(++Dship));
    if(type == 3)
        ui->Csurplus->setText(QString::number(++Cship));
    if(type == 0)
        ui->Bsurplus->setText(QString::number(++Bship));
    //1 s, 2 x 3 z,4 y
    switch(direction)
    {
        case 1:DeleteS(x,y,type,pix);break;
        case 2:DeleteX(x,y,type,pix);break;
        case 3:DeleteZ(x,y,type,pix);break;
        case 4:DeleteY(x,y,type,pix);break;
    }
    ui->delX->clear();
    ui->delY->clear();
}

void Configuration::DeleteS(int x, int y, int type,QPixmap pix)
{
    if(type == 0)
        type = 4;
    for(int i = 1;i < type;i++)
    {
        seaArray[x+i][y] = 0;
        seaField[x+i][y]->setPixmap(pix);
    }
}

void Configuration::DeleteX(int x, int y, int type, QPixmap pix)
{
    if(type == 0)
        type = 4;
    for(int i = 1;i < type;i++)
    {
        seaArray[x-i][y] = 0;
        seaField[x-i][y]->setPixmap(pix);
    }
}

void Configuration::DeleteZ(int x, int y, int type, QPixmap pix)
{
    if(type == 0)
        type = 4;
    for(int i = 1;i < type;i++)
    {
        seaArray[x][y+i] = 0;
        seaField[x][y+i]->setPixmap(pix);
    }
}

void Configuration::DeleteY(int x, int y, int type, QPixmap pix)
{
    if(type == 0)
        type = 4;
    for(int i = 1;i < type;i++)
    {
        seaArray[x][y-i] = 0;
        seaField[x][y-i]->setPixmap(pix);
    }
}
