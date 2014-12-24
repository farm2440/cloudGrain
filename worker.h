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

#include "serialport.h"
#include "iniparser.h"

struct Settings
{
    QString serialPortName;
    QString postURL;
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
    QByteArray data;

    SerialPort sp; //През този порт става връзката по RS-485 към контролерите

    QDomDocument xmlDoc; //файлът settings.xml се зарежда в този обект и садържа индивидуалните потребителски настройки
    Settings settings;
signals:

public slots:
    //Функции за управление на GPIO пина за посока на SN75176
    void uart1_dirTx();
    void uart1_dirRx();

    void tick(void);
    void replyFinished(QNetworkReply* reply);
};

#endif // WORKER_H
