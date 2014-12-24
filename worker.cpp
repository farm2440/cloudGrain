#include "worker.h"

Worker::Worker(QObject *parent) :  QObject(parent)
{
    connect(&tmr, SIGNAL(timeout()), this, SLOT(tick()));
    tmr.start(1000);
    n=1;

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    dataHeader="<string xmlns=\"http://schemas.microsoft.com/2003/10/Serialization/\">";
    dataHeader += "s_stavrev@hotmail.com;";
    dataHeader+="10E36F67-570D-4B82-B4EB-B20831AAAD5B;";
    dataHeader+="494F5931-0A06-45BC-84CB-8F6FD3D67FCD;";

    //Зареждата се настройките от settings.xml
    QFile xmlFile("settings.xml");
    if(! xmlFile.open(QIODevice::ReadOnly))
    {
        std::cerr << "ERROR: Unable to open settings.xml" << std::endl;
        //TODO: Трябва да има healthservice за всички клиенти където да се пращат съобщения
        //за грешки общо от всички клиенти които ние да можем да следим.
    }
    else
    {
        xmlDoc.setContent(&xmlFile);
        xmlFile.close();
/*
        QDomElement root = docRx.documentElement();
        QString rootName = root.tagName();
        if(rootName!="cloudGrain") std::cout << "ERROR: Error parsing XML. Root element must be cloudGrain." << std::endl;

        QDomNodeList childNodesList = root.childNodes();
        if(childNodesList.count()<2)
        {
            qDebug() << "ERR: Error parsing XML. Child nodes missing.";
            return;
        }

        //base
        int base;
        QDomNode baseNode = childNodesList.at(0);
        QDomElement  baseElement = baseNode.toElement();
        if(baseElement.tagName() != "base")
        {
            qDebug() << "ERR: Error parsing XML. First tag must be <base>.";
            return;
        }
        else
        {
            base = baseElement.text().toInt(&ok);
            if(!ok)
            {
                qDebug() << "ERR: Error parsing XML. Failed parsing base.";
                return;
            }
        }*/
    }

    //край зареждане на параметрите от settings.xml


    //Отваряне на сериен порт
    std::cout << "Opening serial port...";
    int h = sp.sp_open("/dev/ttyO1");
    if(h==-1) std::cout << " failed" << std::endl;
    else std::cout << " ok" << std::endl;

    h = sp.sp_setPort(B1200,8,SerialPort::PARITY_NONE,1);
    sp.sp_stopRxTimer();
}

void Worker::tick(void)
{
    std::cout << "tick " << n++ << std::endl;
//Подготовка на данните
    QString strData = dataHeader;
    QString timestamp;
    timestamp = QString::number(55+n%3) + ";";
    timestamp += QDate::currentDate().toString("yyyy-MM-dT");
    timestamp += QTime::currentTime().toString("hh:mm:ss.zzzZ;");

    strData += timestamp;
    strData +="</string>";
    data=strData.toUtf8();

//POST
    //http://qt-project.org/doc/note_revisions/84/235/view
    //QUrl params;
    //params.addQueryItem("key", "value");
    //params.addQueryItem("another_key", "äöutf8-value");

    QUrl site("http://devicewebvarnaservice.azurewebsites.net/RestService/SimpleWriteSingleValueDeviceData");
    QNetworkRequest request(site);
    request.setRawHeader("Content-Type","text/xml;charset=utf-8");

    //if((n%7)==0) manager->post(request, params.encodedQuery());
    if((n%7)==0) manager->post(request, data);

/* //GET
//    if((n%5)==0)   manager->get(QNetworkRequest(QUrl("http://qt-project.org")));
//        if((n%5)==0)   manager->get(QNetworkRequest(QUrl("http://www.etherino.net/index.html")));
  */

    //Serial Port
    if(sp.sp_isOpen()) sp.sp_write("Hello SP!\r\n");
    else std::cout << "Serial is not open!" << std::endl;

    //za proba
    if(n%2) uart1_dirRx();
    else uart1_dirTx();
}


void Worker::replyFinished(QNetworkReply * reply)
{
    std::cout << "reply! " << std::endl;

    QList<QByteArray> hlist = reply->rawHeaderList();
    std::cout << "headers num=" << hlist.count() << std::endl;

    QString name,data;
    foreach(QByteArray ba, hlist)
    {
        name = QString(ba);
        std::cout << " HEADER: " << name.toStdString() << std::endl;

        data = QString(reply->rawHeader(ba));
        std::cout << " DATA: " << data.toStdString() << std::endl;
    }
}

void Worker::uart1_dirRx()
{
    std::fstream fs;
    fs.open("/sys/class/gpio/gpio115/value", std::fstream::out);
    fs << "0";
    fs.close();
}

void Worker::uart1_dirTx()
{
    std::fstream fs;
    fs.open("/sys/class/gpio/gpio115/value", std::fstream::out);
    fs << "1";
    fs.close();
}
