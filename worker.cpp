#include "worker.h"

Worker::Worker(QObject *parent) :  QObject(parent)
{
    connect(&tmr, SIGNAL(timeout()), this, SLOT(tick()));
    tmr.setSingleShot(true);
    n=1;

    ramFile.setFileName("/mnt/ramdisk/sensors");
    spCon1 =  new AbstractSerial();

    //Зареждата се настройките от settings.xml
    QFile xmlFile("settings.xml");
    if(! xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "ERROR: Unable to open settings.xml";
        //TODO: Трябва да има healthservice за всички клиенти където да се пращат съобщения
        //за грешки общо от всички клиенти които ние да можем да следим.
    }
    else
    {
        xmlDoc.setContent(&xmlFile);
        xmlFile.close();

        QDomElement root = xmlDoc.documentElement();
        QString rootName = root.tagName();

        qDebug() << "parsing xml file...";
        qDebug() << "rootName=" << rootName;
        if(rootName!="cloudGrain") qDebug() << "ERROR: Error parsing XML. Root element must be cloudGrain.";

        QDomNodeList childNodesList = root.childNodes();
        for(int i=0 ; i<childNodesList.count() ; i++)
        {
            QDomNode node = childNodesList.at(i);
            QDomElement elm= node.toElement();            
            qDebug() << "node:" << elm.tagName() << ":" << elm.text();
            if(elm.tagName()=="serialPort") settings.serialPort=elm.text();
            if(elm.tagName()=="postURL") settings.postURL=elm.text();
            if(elm.tagName()=="customer") settings.customer=elm.text();
            if(elm.tagName()=="id") settings.id=elm.text();
            if(elm.tagName()=="email") settings.email=elm.text();

            if(elm.tagName()=="readPeriod") settings.readPeriod=elm.text().toInt()*1000;
            if(settings.readPeriod<1000) settings.readPeriod = 1000;

            if(elm.tagName()=="postPeriod") settings.postPeriod=elm.text().toInt();
            if(settings.postPeriod<1) settings.postPeriod=1;

            //Данни за силоза
            if(elm.tagName()=="silo")
            {
                QString siloName = elm.attribute("name","N/A");
                QString siloLocation = elm.attribute("location","N/A");
                settings.siloName=siloName;
                settings.siloLocation=siloLocation;
                qDebug() << "silo name:" << siloName << "  location:" << siloLocation;
                QDomNodeList controllers = node.childNodes();
                //Извличане на данните за контролерите на силоза
                for(int j=0 ; j< controllers.count() ; j++)
                {
                    QDomNode nodeCont = controllers.at(j);
                    QDomElement elmCont = nodeCont.toElement();
                    int address = elmCont.attribute("rs485","-1").toInt();
                    if(address!=-1) settings.controllers.append(elmCont.attribute("rs485",""));
                    QString version = elmCont.attribute("version","N/A");
                    qDebug() << "   controller address:" << address << " firmware version:" << version;
                    //Извличане на данните за сензорите от трите 1-wire шини
                    QDomNodeList sensors = nodeCont.childNodes();
                    qDebug() << "   there are " << sensors.count() << " sensors on controller " << address << "\r\n";
                    for (int n=0 ; n< sensors.count() ; n++)
                    {
                        QString mac,rope,level,guid,secret;
                        QDomNodeList sparams = sensors.at(n).childNodes();
                        for( int m=0 ; m<sparams.count() ; m++)
                        {
                            QDomElement elmParam = sparams.at(m).toElement();
                            if(elmParam.tagName()=="mac") mac=elmParam.text();
                            if(elmParam.tagName()=="rope") rope=elmParam.text();
                            if(elmParam.tagName()=="level") level=elmParam.text();
                            if(elmParam.tagName()=="guid") guid=elmParam.text();
                            if(elmParam.tagName()=="secret") secret=elmParam.text();
                        }                        
                        qDebug() << "   mac:" << mac;
                        qDebug() << "   rope:" << rope;
                        qDebug() << "   level:" << level;
                        qDebug() << "   guid:" << guid;
                        qDebug() << "   secret:" << secret << "\r\n";
                        //Сензора се добавя в списъка
                        Sensor s;
                        s.guid=guid;
                        s.level=level;
                        s.mac=mac;
                        s.rope=rope;
                        s.secret=secret;
                        s.value="N/A";
                        s.timestamp="N/A";
                        listSensors.append(s);
                        //В списъците listRopes И listLevels се добавя ниво и номер на въже ако още не е добавено
                        if(!listRopes.contains(s.rope.toInt())) listRopes.append(s.rope.toInt());
                        if(!listLevels.contains(s.level.toInt())) listLevels.append(s.level.toInt());
                    }
                }
            }
        }
    }

    qDebug() << "readPeriod=" << settings.readPeriod;
    qDebug() << "postPeriod=" << settings.postPeriod;

    //Сортират се номерата на нивата и въжетата. Ползват се при генериране на данните за RAM файла
//    for(int i=0 ; i< listLevels.count() ; i++) qDebug() << "listLevels[" << i << "]=" << listLevels[i];
//    for(int i=0 ; i< listRopes.count() ; i++) qDebug() << "listRopes[" << i << "]=" << listRopes[i];
    qDebug() << "sorting....";
    qSort(listLevels.begin(), listLevels.end(), qGreater<int>());
    qSort(listRopes);
    for(int i=0 ; i< listLevels.count() ; i++) qDebug() << "listLevels[" << i << "]=" << listLevels[i];
    for(int i=0 ; i< listRopes.count() ; i++) qDebug() << "listRopes[" << i << "]=" << listRopes[i];
    qDebug() << " ";
    //край зареждане на параметрите от settings.xml

    //Отваряне на сериен порт
    uart1_dirRx();
    qDebug() << "Opening serial port...";
    spCon1->setDeviceName(settings.serialPort);
    if (spCon1->open(QIODevice::ReadWrite | QIODevice::Unbuffered))
    {
            qDebug() << "Serial device " << spCon1->deviceName() << " open in " << spCon1->openMode();
            spCon1->setBaudRate(AbstractSerial::BaudRate1200);
            spCon1->setDataBits(AbstractSerial::DataBits8);
            spCon1->setParity(AbstractSerial::ParityNone);
    }
    else qDebug() << "ERROR: Failed to open UART for connetion to ThermoLog controller!";

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    request.setUrl(settings.postURL);

    dataHeader="<string xmlns=\"http://schemas.microsoft.com/2003/10/Serialization/\">";
    dataHeader += (settings.email + ";");

    tmr.start(10000);
}

/*
Worker::~Worker()
{
    spCon1->close();
    delete spCon1;
}
*/

void Worker::tick(void)
{

    QString rxdata;
    QByteArray rxBytes;

    qDebug() << "\r\ntick " << n++;
    //Serial Port
    if(!spCon1->isOpen()) qDebug() << "Serial " << spCon1->deviceName() << " is not open!";

    timestamp = QDate::currentDate().toString("yyyy-MM-dT");
    timestamp += QTime::currentTime().toString("hh:mm:ss.zzzZ;");

    //Прочитане на сензорите
    for(int i=0 ; i<settings.controllers.count() ; i++)
    {
        for(int bus=0 ; bus<3 ; bus++)
        {
            uart1_dirTx();
            spCon1->write(QString("tst %1 %2\r\n").arg(settings.controllers[i]).arg(bus).toLatin1());
            spCon1->flush();
            uart1_dirRx();
            time.start();
//            spCon1->readAll(); //clear buffer
            rxdata="";
            while(true)
            {
                if(spCon1->bytesAvailable(false)>0)
                {
                    rxBytes.clear();
                    rxBytes = spCon1->readAll();
                    rxdata += QString(rxBytes);
                    if(rxdata.contains("\r\n"))
                    {
                        int p = rxdata.indexOf("\r\n");
                        rxdata = rxdata.left(p);
                        qDebug() << "rx:" <<rxdata;
                        break;
                    }
                }
                else usleep(20000);
                if(time.elapsed()>2000)
                {
                    qDebug() << "timeout!";
                    break;
                }

            }
            //В приетите данни по RS-485 се търсят стойности от сензорите
            for(int sensIdx=0 ; sensIdx<listSensors.count() ; sensIdx++)
            {
                if(rxdata.contains("[" + listSensors[sensIdx].mac + "]="))
                {
                    int p=rxdata.indexOf("[" + listSensors[sensIdx].mac + "]=");
                    listSensors[sensIdx].value = rxdata.mid(p+11,4);
                    listSensors[sensIdx].value = listSensors[sensIdx].value.insert(2,'.');
                    listSensors[sensIdx].timestamp=timestamp;
                    qDebug() << "value for mac " << listSensors[sensIdx].mac << " is " <<listSensors[sensIdx].value;
                }
            }

        }
    }

    exportRamFile();

//Подготовка на данните  за POST
    QString strData = dataHeader;
    //DUMMY SENSOR
    strData +="10E36F67-570D-4B82-B4EB-B20831AAAD5B;"; //guid - dummy sensor
    strData +="494F5931-0A06-45BC-84CB-8F6FD3D67FCD;"; //secret - dummy sensor
    strData += QString::number(25+n%5) + ";"; //стойност
    strData += timestamp;
    strData +="</string>";
    postData.clear();
    postData=strData.toUtf8();

    request.setRawHeader("Content-Type","text/xml;charset=utf-8");
    if((n % settings.postPeriod)==0)
    {
        manager->post(request, postData);
        qDebug() << "POST:" << strData;
    }

    //Истински сензори
    foreach(Sensor sens, listSensors)
    {
        strData =  dataHeader;
        strData += (sens.guid + ";");
        strData += (sens.secret + ";");
        strData += (sens.value + ";");
        strData += timestamp;

        strData +="</string>";
        postData.clear();
        postData=strData.toUtf8();
        request.setRawHeader("Content-Type","text/xml;charset=utf-8");
        if((n % settings.postPeriod)==0)
        {
            manager->post(request, postData);
            qDebug() << "POST:" << strData;
        }
    }

    tmr.start(settings.readPeriod);
}


void Worker::replyFinished(QNetworkReply * reply)
{
    qDebug() <<"\r\nreply! ";

    QList<QByteArray> hlist = reply->rawHeaderList();
    qDebug() << "headers num=" << hlist.count();

    QString name,data;
    foreach(QByteArray ba, hlist)
    {
        name = QString(ba);
        qDebug() << " HEADER: " << name;

        data = QString(reply->rawHeader(ba));
        qDebug() << " DATA: " << data;
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
    usleep(5000);
}

bool Worker::exportRamFile()
{//връща true при успех иначе false
    /*в /mnt/ramdisk е монтирана от стартиращия скрипт RAM-базирана файлова система.
      Файлът /mnt/ramdisk/sensors съдържа данните за настройки и актуални стойности на сензорите
    */

    ramFile.open(QIODevice::WriteOnly);

    QString data= "Company: " + settings.customer + "<br>\r\n";
    data += ("Silo: " + settings.siloName + "<br>\r\n");
    data += ("Location: " + settings.siloLocation + "<br><br>\r\n");
    data += ("Timestamp: " + timestamp + "<br><br><hl><br>\r\n");

    data +=  "<table  border=\"1\"> \r\n";
    data += ("  <caption>Sensor values:</caption>\r\n");

    //първи ред в таблицата - хедъри
    data += ("<tr><th>level/rope</th>");
    for(int r=0 ; r<listRopes.count() ; r++) data+=("<th>" + QString::number(listRopes[r]) + "</th>");
    data += "</tr>\r\n";
    //Останалите редове в таблицата - първа колона е номер на ниво останалите са температура на датчиците
    for(int l=0 ; l<listLevels.count() ; l++)
    {
        data += ("<tr><th>" + QString::number(listLevels[l]) +"</th>");//Номер на ниво
        for(int r=0 ; r<listRopes.count() ; r++)
        {
            data += ("<td>" + getSensorValue(listRopes[r],listLevels[l])+ "</td>");
        }
        data += "</tr>\r\n";
    }
    data += "</table>";
    ramFile.write(data.toLatin1());
    ramFile.close();
    return true;
}

QString Worker::getSensorValue(int rope, int level)
{//Връща стойността за сензор от listSensors
    foreach(Sensor s, listSensors)
    {
        if((s.rope.toInt()==rope) && (s.level.toInt()==level))
        {
//            qDebug() << "rope:" << rope << "  level:" <<level << "  value:" <<s.value;
            return s.value;
        }
    }


    return "---";
}
