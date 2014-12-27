#ifndef WORKER_H
#define WORKER_H

#include <QObject>

#include <QTimer>
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QTextStream>
#include <QTime>
#include <QDate>


#include <QFile>
#include <QtXml>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <iostream>
#include <fstream>
#include <unistd.h> //usleep()

#include "serialport.h"
#include "iniparser.h"

struct Settings
{
    QString serialPort;
    QString postURL;
    QString customer;
    QString id;
    QString email;
    QString siloName;
    QString siloLocation;

    QByteArray controllers; //съдържа RS-485 адресите на свързаните контролери
};

struct Sensor
{
    QString mac;
    QString rope;
    QString level;
    QString guid;
    QString secret;

    QString value;
    QString timestamp;
};

class Worker : public QObject
{
Q_OBJECT
public:

    explicit Worker(QObject *parent = 0);

    QTimer tmr;
    int n;

    //За POST изпращане на данни към облака
    QNetworkAccessManager *manager;
    QString dataHeader;

    SerialPort sp; //През този порт става връзката по RS-485 към контролерите
    char rxbuf[200];

    QDomDocument xmlDoc; //файлът settings.xml се зарежда в този обект и садържа индивидуалните потребителски настройки
    Settings settings;
    QList<Sensor> listSensors;


signals:

public slots:
    //Функции за управление на GPIO пина за посока на SN75176
    void uart1_dirTx();
    void uart1_dirRx();

    void tick(void);
    void replyFinished(QNetworkReply* reply);
};

#endif // WORKER_H
