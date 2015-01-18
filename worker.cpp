#include "worker.h"

Worker::Worker(QObject *parent) :  QObject(parent)
{
    exportRamFile_Error("WARNING: Starting the service. Please wait...");
    line1.setDeviceName("/dev/ttyO1");
    line2.setDeviceName("/dev/ttyO2");
    line3.setDeviceName("/dev/ttyO4");

    //Зареждата се настройките от settings.xml
    QFile xmlFile("settings.xml");
    if(! xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "ERROR: Unable to open settings.xml";
        //TODO: Трябва да има healthservice за всички клиенти където да се пращат съобщения
        //за грешки общо от всички клиенти които ние да можем да следим.
        exportRamFile_Error("ERROR:Unable to open settings xml file");
        while(1){}
    }
    else
    {
        xmlDoc.setContent(&xmlFile);
        xmlFile.close();

        QDomElement root = xmlDoc.documentElement();
        QString rootName = root.tagName();

        qDebug() << "parsing xml file...";
        qDebug() << "rootName=" << rootName;
        if(rootName!="cGate")
        {
            qDebug() << "ERROR: Failed parsing XML. Root element must be cGate.";
            exportRamFile_Error("ERROR: Failed parsing XML. Root element must be cGate.");
            while(1){}
        }

        QDomNodeList childNodesList = root.childNodes();
        for(int i=0 ; i<childNodesList.count() ; i++)
        {
            QDomNode node = childNodesList.at(i);
            QDomElement elm= node.toElement();            
            qDebug() << "node:" << elm.tagName() << ":" << elm.text();
            if(elm.tagName()=="configName") settings.configName=elm.text();
            if(elm.tagName()=="postURL") settings.postURL=elm.text();
            if(elm.tagName()=="customer") settings.customer=elm.text();
            if(elm.tagName()=="customerId") settings.customerId=elm.text();
            if(elm.tagName()=="email") settings.email=elm.text();
            if(elm.tagName()=="locationName") settings.locationName=elm.text();
            if(elm.tagName()=="locationId") settings.locationId=elm.text();

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
                settings.siloName = elm.attribute("siloName","N/A");
                qDebug() << "silo name:" << settings.siloName;
                QDomNodeList controllers = node.childNodes();
                //Извличане на данните за контролерите на силоза
                for(int j=0 ; j< controllers.count() ; j++)
                {
                    QDomNode nodeCont = controllers.at(j);
                    QDomElement elmCont = nodeCont.toElement();

                    Controller ctrl;
                    ctrl.address = elmCont.attribute("rs485","-1").toInt();
                    ctrl.line = elmCont.attribute("line","-1").toInt();
                    ctrl.version = elmCont.attribute("version","N/A");
                    if((ctrl.address<0)||(ctrl.address>15))
                    {
                        qDebug() << "ERROR: Invalid controller address!";
                        continue;
                    }
                    if((ctrl.line<1)||(ctrl.line>3))
                    {
                        qDebug() << "ERROR: Invalid controller line number!";
                        continue;
                    }

                    listControllers.append(ctrl);
                    qDebug() << "   controller address:" << ctrl.address << "  line:" << ctrl.line << " firmware version:" << ctrl.version;
                    //Извличане на данните за сензорите от трите 1-wire шини
                    QDomNodeList sensors = nodeCont.childNodes();
                    qDebug() << "   there are " << sensors.count() << " sensors on controller " << ctrl.address << "on line " << ctrl.line << "\r\n";
                    for (int n=0 ; n< sensors.count() ; n++)
                    {
                        QString mac,rope,level,guid,secret,type;
                        QDomNodeList sparams = sensors.at(n).childNodes();
                        for( int m=0 ; m<sparams.count() ; m++)
                        {
                            QDomElement elmParam = sparams.at(m).toElement();
                            if(elmParam.tagName()=="mac") mac=elmParam.text();
                            if(elmParam.tagName()=="rope") rope=elmParam.text();
                            if(elmParam.tagName()=="level") level=elmParam.text();
                            if(elmParam.tagName()=="guid") guid=elmParam.text();
                            if(elmParam.tagName()=="secret") secret=elmParam.text();
                            if(elmParam.tagName()=="type") type=elmParam.text();
                        }
                        if(mac=="") continue;
                        qDebug() << "   mac:" << mac;
                        qDebug() << "   rope:" << rope;
                        qDebug() << "   level:" << level;
                        qDebug() << "   type:" << type;
                        qDebug() << "   guid:" << guid;
                        qDebug() << "   secret:" << secret << "\r\n";
                        //Сензора се добавя в списъка
                        Sensor s;
                        s.guid=guid;
                        s.level=level;
                        s.mac=mac;
                        s.type=type;
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
    qDebug() << "mcastPeriod=" << settings.mcastPeriod;
    postCounter=settings.postPeriod;
    mcastCounter=settings.mcastPeriod;

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

    exportRamFile_Version();
    exportRamFile_Settings();

    //Отваряне на сериините портове
    setLineDirRx(1);
    qDebug() << "Opening serial port for line 1...";
    if (line1.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
    {
            qDebug() << "Serial device " << line1.deviceName() << " open in " << line1.openMode();
            line1.setBaudRate(AbstractSerial::BaudRate1200);
            line1.setDataBits(AbstractSerial::DataBits8);
            line1.setParity(AbstractSerial::ParityNone);
    }
    else qDebug() << "ERROR: Failed to open UART for line 1";

    setLineDirRx(2);
    qDebug() << "Opening serial port for line 2...";
    if (line2.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
    {
            qDebug() << "Serial device " << line2.deviceName() << " open in " << line2.openMode();
            line2.setBaudRate(AbstractSerial::BaudRate1200);
            line2.setDataBits(AbstractSerial::DataBits8);
            line2.setParity(AbstractSerial::ParityNone);
    }
    else qDebug() << "ERROR: Failed to open UART for line 2";

    setLineDirRx(3);
    qDebug() << "Opening serial port for line 3...";
    if (line3.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
    {
            qDebug() << "Serial device " << line3.deviceName() << " open in " << line3.openMode();
            line3.setBaudRate(AbstractSerial::BaudRate1200);
            line3.setDataBits(AbstractSerial::DataBits8);
            line3.setParity(AbstractSerial::ParityNone);
    }
    else qDebug() << "ERROR: Failed to open UART for line 3";
    line1.close();
    line2.close();
    line3.close();

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    request.setUrl(settings.postURL);

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
    AbstractSerial *pLine;
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

    //HEARTBEAT
    //На всеки цикъл състоянието на този пин се обръща и се нулира външно свързан хардуерен WATCHDOG
    std::fstream heartbeat;
    heartbeat.open("/sys/class/gpio/gpio50/value", std::fstream::out);
    if((loopCounter%2)==0) heartbeat << "0";
    else heartbeat << "1";
    heartbeat.close();

    timestamp = QDate::currentDate().toString("yyyy-MM-dT");
    timestamp += QTime::currentTime().toString("hh:mm:ss.zzzZ;");
    for(int sensIdx=0 ; sensIdx<listSensors.count() ; sensIdx++) listSensors[sensIdx].value = "N/A";
    //Прочитане на сензорите
    foreach(Controller ctrl, listControllers)
    {
        switch(ctrl.line)
        {
        case 1:
            pLine = &line1;
            break;
        case 2:
            pLine = &line2;
            break;
        case 3:
            pLine = &line3;
            break;
        default:
            qDebug() << "ERROR: Invalid value for controller's line!";
            continue;
        }

        if((ctrl.address<0)||(ctrl.address>15))
        {
            qDebug() << "ERROR: Invalid controller address!";
            continue;
        }

        pLine->open(QIODevice::ReadWrite);
        for(int bus=0 ; bus<3 ; bus++)
        {
            setLineDirTx(ctrl.line);
            pLine->write(QString("tst %1 %2\r\n").arg(ctrl.address).arg(bus).toLatin1());
            pLine->flush();
            setLineDirRx(ctrl.line);
            time.start();
            rxdata="";
            while(true)
            {
                if(pLine->bytesAvailable(false)>0)
                {
                    rxBytes.clear();
                    rxBytes = pLine->readAll();
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
                    listSensors[sensIdx].value = rxdata.mid(p+3+listSensors[sensIdx].mac.length(),4);
                    listSensors[sensIdx].value = listSensors[sensIdx].value.insert(2,'.');
                    listSensors[sensIdx].timestamp=timestamp;
                    qDebug() << "value for mac " << listSensors[sensIdx].mac << " is " <<listSensors[sensIdx].value;
                }
            }
        }
        pLine->close();
    }

    qDebug() << "exportRamFile with timestamp:" << timestamp;
    exportRamFile_LiveDataTable(timestamp);
    exportRamFile_SensorsTable(timestamp);

    //Подготовка на данните  за POST
    qDebug() << "postCounter=" <<postCounter << "   mcastCounter=" << mcastCounter;
    if(settings.postPeriod!=0)
    {
        postCounter--;
        if(postCounter==0)
        {
            qDebug() << "sending POST...";
            postCounter=settings.postPeriod;
            QString strData = dataHeader;
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
                //qDebug() << "POST:" << strData;
            }
        }
    }//if(settings.postPeriod!=0)

    //multicast
    if(settings.mcastPeriod!=0)
    {
        mcastCounter--;
        if(mcastCounter==0)
        {
            qDebug() << "Sending multicast...";
            mcastCounter=settings.mcastPeriod;
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
    qDebug() << "multicast sent.";

    //Рестарт на таймера - цикъла е затворен
    timer.start(settings.readPeriod*1000);
}

void Worker::replyFinished(QNetworkReply * reply)
{
/*
    qDebug() <<"post reply!";

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

    QByteArray respData = reply->readAll();
    qDebug() << "respData: " << QString(respData);
*/
    //!!!!!!!!
    reply->deleteLater();
}

void Worker::setLineDirRx(int line)
{
    std::fstream fs;
    switch(line)
    {
    case 1:
        fs.open("/sys/class/gpio/gpio115/value", std::fstream::out);
        break;
    case 2:
        fs.open("/sys/class/gpio/gpio49/value", std::fstream::out);
        break;
    case 3:
        fs.open("/sys/class/gpio/gpio60/value", std::fstream::out);
        break;
    }
    fs << "0";
    fs.close();
}

void Worker::setLineDirTx(int line)
{
    std::fstream fs;
    switch(line)
    {
    case 1:
        fs.open("/sys/class/gpio/gpio115/value", std::fstream::out);
        break;
    case 2:
        fs.open("/sys/class/gpio/gpio49/value", std::fstream::out);
        break;
    case 3:
        fs.open("/sys/class/gpio/gpio60/value", std::fstream::out);
        break;
    }
    fs << "1";
    fs.close();
    usleep(5000);
}

bool Worker::exportRamFile_LiveDataTable(QString timestamp)
{//връща true при успех иначе false

    /* Генерира се html таблица която съдържа актуални данни от сензорите.
       След като е готово съдържанието на html-а  то се записва във файл във RAM паметта.
       В /mnt/ramdisk е монтирана от стартиращия скрипт RAM-базирана файлова система.
       Файлът /mnt/ramdisk/livedatatable.html съдържа данните за настройки и актуални стойности на сензорите
       Този файл ще се импортира като фрейм в страницата livedata на уеб интерфейса
    */


    //Генериране на таблица с актуални стойности
    QString table;
    QString _timestamp = timestamp.left(timestamp.indexOf('T')) + "   " + timestamp.mid(timestamp.indexOf('T')+1, 8);

    table = "<div id=\"siloId\">\r\n";
    table += ("      Silo: <b>" + settings.siloName + "</b><br>\r\n");
    table += ("  Location: <b>" + settings.locationName + "</b><br>\r\n");
    table += "</div>";

    table +=  "<table id=\"liveDataTable\"> \r\n";
    table += ("  <caption>" + _timestamp + "</caption>\r\n");

    //първи ред в таблицата - хедъри
    table += ("<tr><th  id=\"ld\">level/rope</th>");
    for(int r=0 ; r<listRopes.count() ; r++) table+=("<th  id=\"ld\">" + QString::number(listRopes[r]) + "</th>");
    table += "</tr>\r\n";
    //Останалите редове в таблицата - първа колона е номер на ниво останалите са температура на датчиците
    for(int l=0 ; l<listLevels.count() ; l++)
    {
        table += ("<tr><th  id=\"ld\">" + QString::number(listLevels[l]) +"</th>");//Номер на ниво
        for(int r=0 ; r<listRopes.count() ; r++)
        {
            table += ("<td  id=\"ld\">" + getSensorValue(listRopes[r],listLevels[l])+ "</td>");
        }
        table += "</tr>\r\n";
    }
    table += "</table>";

    //Готовия html се записва в RAM файла
    QFile ramFile("/mnt/ramdisk/livedatatable.html");
    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/livedatatable.html";
        return false;
    }
    ramFile.write(table.toLatin1());
    ramFile.close();
    return true;
}

bool Worker::exportRamFile_SensorsTable(QString timestamp)
{
    /* Генерира се html таблица която съдържа актуални данни от сензорите.
       След като е готово съдържанието на html-а  то се записва във файл във RAM паметта.
       В /mnt/ramdisk е монтирана от стартиращия скрипт RAM-базирана файлова система.
       Файлът /mnt/ramdisk/sensorstable.html съдържа данните за настройки и актуални стойности на сензорите
       Този файл ще се импортира като фрейм в страницата livedata на уеб интерфейса
    */

    //Генериране на таблица с актуални стойности
    QString table;
    QString _timestamp = timestamp.left(timestamp.indexOf('T')) + "   " + timestamp.mid(timestamp.indexOf('T')+1, 8);
    //Генерира се и таблица с МАС адресите, типа, мястото и измерената стойност
    table = "<div id=\"siloId\">\r\n";
    table += ("      Silo: <b>" + settings.siloName + "</b><br>\r\n");
    table += ("  Location: <b>" + settings.locationName + "</b><br>\r\n");
    table += "</div>";

    table +=  "<table id=\"liveDataTable\"> \r\n";
    table += ("  <caption>" + _timestamp + "</caption>\r\n");

    //първи ред в таблицата - хедъри
    table += ("<tr><th id=\"ld\">N</th>");
    table += ("<th id=\"ld\">MAC</th>");
    table += ("<th id=\"ld\">T</th>");
    table += ("<th id=\"ld\">R</th>");
    table += ("<th id=\"ld\">L</th>");
    table += ("<th id=\"ld\">VALUE</th>");
    table += "</tr>\r\n";
    int sensIdx=1;
    //Останалите редове в таблицата - първа колона е номер на ниво останалите са температура на датчиците
    for(int l=0 ; l<listLevels.count() ; l++)
    {
        for(int r=0 ; r<listRopes.count() ; r++)
        {
            QString mac=getSensorMac(listRopes[r],listLevels[l]);
            if(mac=="---") continue;
            table += ("<tr><th id=\"ld\">" + QString::number(sensIdx) +"</th>");//Номер на ниво
            sensIdx++;
            table += ("<td id=\"ld\">" + getSensorMac(listRopes[r],listLevels[l])+ "</td>");
            table += ("<td id=\"ld\">" + getSensorType(listRopes[r],listLevels[l])+ "</td>");
            table += ("<td id=\"ld\">" + QString::number(listRopes[r]) + "</td>");
            table += ("<td id=\"ld\">" + QString::number(listLevels[l]) + "</td>");
            table += ("<td id=\"ld\">" + getSensorValue(listRopes[r],listLevels[l])+ "</td>");
        }
        table += "</tr>\r\n";
    }
    table += "</table>";
    //Готовия html се записва в RAM файла
    QFile ramFile("/mnt/ramdisk/sensorstable.html");
    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/livedatatable.html";
        return false;
    }
    ramFile.write(table.toLatin1());
    ramFile.close();
    return true;
}

bool Worker::exportRamFile_Settings()
{
    //Генериране на таблица с актуални стойности
    QString table;

    table +=  "<table id=\"liveDataTable\"> \r\n";
    table += ("  <caption>Configuration name: " + settings.configName + "</caption>\r\n");

    //първи ред в таблицата - хедъри
    table += ("  <tr><th id=\"ld\">Parameter</th><th id=\"ld\">Value</th><tr>\r\n");
    table += ("  <tr><td id=\"ld\">Custormer</td><td id=\"ld\">" + settings.customer + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">e-mail</td><td id=\"ld\">" + settings.email + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">POST Url</td><td id=\"ld\">" + settings.postURL + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">Multicast address</td><td id=\"ld\">" + settings.groupAddress.toString() + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">Multicast port</td><td id=\"ld\">" + QString::number(settings.groupPort) + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">readPeriod</td><td id=\"ld\">" + QString::number(settings.readPeriod) + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">postPeriod</td><td id=\"ld\">" + QString::number(settings.postPeriod) + "</td></tr>\r\n");
    table += ("  <tr><td id=\"ld\">mcastPeriod</td><td id=\"ld\">" + QString::number(settings.mcastPeriod) + "</td></tr>\r\n");
    //table += ("  <tr><td id=\"ld\"> </td><td>" +  + "</td id=\"ld\"></tr>\r\n");
    table += "</table>\r\n";


    //Готовия html се записва в RAM файла
    QFile ramFile("/mnt/ramdisk/settings.html");
    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/settings.html";
        return false;
    }
    ramFile.write(table.toLatin1());
    ramFile.close();
    return true;
}

bool Worker::exportRamFile_Version()
{
    QString version;
    QFile ramFile("/mnt/ramdisk/version.html");

    version = "firmware v." + QString::number(STR_VERSION) + '.' + QString::number(STR_SUBVERSION) + '.' + QString::number(STR_COMPILATION);

    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/version.html";
        return false;
    }
    ramFile.write(version.toLatin1());
    ramFile.close();
    return true;
}

bool Worker::exportRamFile_Error(QString errMsg)
{

    QString htmlStart = "<div id=\"errMsg\"><b>";
    QString htmlEnd = "</b></div>";
    QString html = htmlStart + errMsg + htmlEnd;

    QFile ramFile("/mnt/ramdisk/livedatatable.html");
    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/settings.html";
    }
    else
    {
        ramFile.write(html.toLatin1());
        ramFile.close();
    }

    ramFile.setFileName("/mnt/ramdisk/settings.html");
    if(!ramFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: failed opening file in RAM /mnt/ramdisk/settings.html";
    }
    else
    {
        ramFile.write(html.toLatin1());
        ramFile.close();
    }
    return true;
}

QString Worker::getSensorValue(int rope, int level)
{//Връща стойността за сензор от listSensors
    foreach(Sensor s, listSensors)
    {
        if((s.rope.toInt()==rope) && (s.level.toInt()==level))     return s.value;
    }
    return "---";
}

QString Worker::getSensorMac(int rope, int level)
{//Връща стойността за сензор от listSensors
    foreach(Sensor s, listSensors)
    {
        if((s.rope.toInt()==rope) && (s.level.toInt()==level))     return s.mac;
    }
    return "---";
}

QString Worker::getSensorType(int rope, int level)
{//Връща стойността за сензор от listSensors
    foreach(Sensor s, listSensors)
    {
        if((s.rope.toInt()==rope) && (s.level.toInt()==level))     return s.type;
    }
    return "---";
}
