#include "worker.h"

Worker::Worker(QObject *parent) :  QObject(parent)
{
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

            if(elm.tagName()=="readPeriod") settings.readPeriod=elm.text().toInt();
            if(settings.readPeriod<1) settings.readPeriod = 1;

            if(elm.tagName()=="postPeriod") settings.postPeriod=elm.text().toInt();
            if(settings.postPeriod==0) qDebug() << "WARNING: postPeriod=0 - no data will be streamed to web!";
            if(settings.postPeriod<0) settings.postPeriod=1;

            if(elm.tagName()=="mcastPeriod") settings.mcastPeriod=elm.text().toInt();
            if(settings.mcastPeriod==0) qDebug() << "WARNING: mcastPeriod=0 - no multicast ill be streamed!";
            if(settings.mcastPeriod<0) settings.mcastPeriod=1;

            if(elm.tagName()=="groupAddress") settings.groupAddress=QHostAddress(elm.text());
            if(elm.tagName()=="groupPort") settings.groupPort=elm.text().toInt();

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

    //udpSocket.setSocketOption(QAbstractSocket::MulticastTtlOption, 1);

    //Старта е отложен за да се даде време за DHCP и NTP
    loopCounter = 1;
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    timer.setSingleShot(true);
    timer.start(10*1000);
}

/*
Worker::~Worker()
{
    spCon1->close();
    delete spCon1;
}
*/

void Worker::timerTick(void)
{
    QString rxdata;
    QByteArray rxBytes;    
    QString timestamp; // Дата/час на последното прочитане на сензорите
    QByteArray datagram;
    QString strDatagram;
    QByteArray postData;
    QString dataHeader;
    dataHeader="<string xmlns=\"http://schemas.microsoft.com/2003/10/Serialization/\">";
    dataHeader += (settings.email + ";");

    qDebug() << "\r\ntick " << loopCounter++;
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

    qDebug() << "exportRamFile with timestamp:" << timestamp;
    exportRamFile(timestamp);

    //Подготовка на данните  за POST
    if(settings.postPeriod!=0)
    {
        if((loopCounter % settings.postPeriod)==0)
        {
            QString strData = dataHeader;
            //DUMMY SENSOR
            strData +="10E36F67-570D-4B82-B4EB-B20831AAAD5B;"; //guid - dummy sensor
            strData +="494F5931-0A06-45BC-84CB-8F6FD3D67FCD;"; //secret - dummy sensor
            strData += QString::number(25+loopCounter%5) + ";"; //стойност
            strData += timestamp;
            strData +="</string>";
            postData.clear();
            postData=strData.toUtf8();

            request.setRawHeader("Content-Type","text/xml;charset=utf-8");

            manager->post(request, postData);
            qDebug() << "POST (dummy sensor):" << strData;

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
                manager->post(request, postData);
                qDebug() << "POST:" << strData;
            }
        }
    }//if(settings.postPeriod!=0)

    //multicast
    if(settings.mcastPeriod!=0)
    {
        if((loopCounter % settings.mcastPeriod)==0)
        {
            foreach(Sensor sens, listSensors)
            {
                strDatagram = "<sensor>\r\n";
                strDatagram += (" <type>" + sens.type + "</type>\r\n");
                strDatagram += (" <rope>" + sens.rope + "</rope>\r\n");
                strDatagram += (" <level>" + sens.level + "</level>\r\n");
                strDatagram += (" <mac>" + sens.mac + "</mac>\r\n");
                strDatagram += (" <value>" + sens.value + "</value>\r\n");
                strDatagram += (" <timestamp>" + sens.timestamp + "</timestamp>\r\n");
                strDatagram += (" <silo>" + settings.siloName + "</silo>\r\n");
                strDatagram += "<sensor>";
                datagram.clear();
                datagram = strDatagram.toLatin1();
                udpSocket.writeDatagram(datagram,settings.groupAddress,settings.groupPort);
            }            
        }
    }


    //Рестарт на таймера - цикъла е затворен
    timer.start(settings.readPeriod*1000);
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

    hlist.clear();
    //!!!!!!!!
    reply->deleteLater();
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

bool Worker::exportRamFile(QString timestamp)
{//връща true при успех иначе false

    /* Генерира се html таблица която съдържа актуални данни от сензорите.
       След като е готово съдържанието на html-а  то се записва във файл във RAM паметта.
       В /mnt/ramdisk е монтирана от стартиращия скрипт RAM-базирана файлова система.
       Файлът /mnt/ramdisk/sensors.html съдържа данните за настройки и актуални стойности на сензорите
       Този файл ще се импортира като фрейм в страницата livedata на уеб интерфейса
    */


    //Генериране на таблица с актуални стойности
    QString table;
    QString _timestamp = timestamp.left(timestamp.indexOf('T')) + "   " + timestamp.mid(timestamp.indexOf('T')+1, 8);

    table +=  "<table id=\"sensorsTable\"> \r\n";
    table += ("  <caption>" + _timestamp + "</caption>\r\n");

    //първи ред в таблицата - хедъри
    table += ("<tr><th>level/rope</th>");
    for(int r=0 ; r<listRopes.count() ; r++) table+=("<th>" + QString::number(listRopes[r]) + "</th>");
    table += "</tr>\r\n";
    //Останалите редове в таблицата - първа колона е номер на ниво останалите са температура на датчиците
    for(int l=0 ; l<listLevels.count() ; l++)
    {
        table += ("<tr><th>" + QString::number(listLevels[l]) +"</th>");//Номер на ниво
        for(int r=0 ; r<listRopes.count() ; r++)
        {
            table += ("<td>" + getSensorValue(listRopes[r],listLevels[l])+ "</td>");
        }
        table += "</tr>\r\n";
    }
    table += "</table>";


    //Готовия html се записва в RAM файла
    QFile ramFile("/mnt/ramdisk/sensors.html");
    ramFile.open(QIODevice::WriteOnly);
    ramFile.write(table.toLatin1());
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
