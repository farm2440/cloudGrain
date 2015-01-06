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

#include <QHostAddress>
#include <QUdpSocket>
#include <QAbstractSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

//#include <iostream>
#include <fstream>
#include <unistd.h> //usleep()

#include <abstractserial.h>

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
    int readPeriod; //секунди между две прочитания на контролера
    int postPeriod; //през колко прочитания на контролера да се пращат данни в Интернет. Ако е 0 не се пращат данни
    int mcastPeriod;//през колко прочитания на контролера да се праща мултикаст в локалната межа. Ако е 0 не се пращат данни

    QHostAddress groupAddress;
    int groupPort;
};

struct Sensor
{
    QString mac;
    QString rope;
    QString level;
    QString guid;
    QString secret;
    QString type;

    QString value;
    QString timestamp;
};

class Worker : public QObject
{
Q_OBJECT
public:

    explicit Worker(QObject *parent = 0);

    AbstractSerial *spCon1;
    char rxbuf[200];

    QDomDocument xmlDoc; //файлът settings.xml се зарежда в този обект и садържа индивидуалните потребителски настройки
    Settings settings;
    QList<Sensor> listSensors;
    QList<int> listRopes;// Списък с номерата на въжетата прочетени от settings.xml
    QList<int> listLevels;// Списък с нивата на сензорите прочетени от settings.xml
                          //най-долния сензор е ниво 0 и растат нагоре
    QTime time;

    QTimer timer;
    int loopCounter; //увеличава се с 1 при всеки цикъл на четене/пращане на данни
    int postCounter, mcastCounter; // Намалят с 1 на всеки цикъл. Като станат 0 се пращат данни
                                   // а стойността се възстановява

    //За POST изпращане на данни към облака
    QNetworkAccessManager *manager;
    QNetworkRequest request;

    QUdpSocket udpSocket;
signals:

public slots:
    //Функции за управление на GPIO пина за посока на SN75176
    void uart1_dirTx();
    void uart1_dirRx();

    void timerTick(void);
    void replyFinished(QNetworkReply* reply);

    bool exportRamFile(QString timestamp); //записва актуални данни в /mnt/ramdisk/sensors
    QString getSensorValue(int rope, int level); //Връща стойността за сензор от listSensors    
};

#endif // WORKER_H
