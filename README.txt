Derek Molloy
Beaglebone: Qt Creator for C++ ARM Embedded Linux Development :
https://www.youtube.com/watch?v=kP7uvOu9hoQ

Building and setting up QT environment for BeagleBone
http://www.cloud-rocket.com/2013/07/building-qt-for-beaglebone/

http://download.qt-project.org/development_releases/
------------------------------------------------------------------------------------
Пускане на NTP:

http://derekmolloy.ie/automatically-setting-the-beaglebone-black-time-using-ntp/
------------------------------------------------------------------------------------
Ползване на UART:

Конзолата е /dev/ttyO0.
За да излезе като /dev/ttyOх  съответния порт трябва да се въведе в конзолата:
 echo BB-UART1 > /sys/devices/bone_capemgr.*/slots

Забележка: в /lib/firmware има копилирани овърлей за UART1,2,4 и 5 и с горната команда могат да се
активират. За да се ползват GPIO пинове за управление посоката на RS-485 обаче трябва да си направя
собствен файл и да го компилирам. По-долу пише как.

Когато връзката е към RS-485 трябва да се управлява сигнала за посока на драйвера SN75176
На страницата:
http://derekmolloy.ie/gpios-on-the-beaglebone-black-using-device-tree-overlays/
Има инструкция и видео как се пуска пин като GPIO. В папката на проекта тук съм клонирал с:
 git clone git://github.com/derekmolloy/boneDeviceTree.git
 проекта от видеото и в папка ~/cloudGrain/boneDeviceTree/overlay има 2 файла които
 трябва да се качат на BBB :
 STIV-GPIO.dts - некомпилиран оверлей с дефиниции за режима на пиновете (текстов файл)
 build-STIV-GPIO - скрипт с команда за компилиране на горния файл.
 и да се стартира скрипта за компилиране

1. в ВВВ се експортват глобални променливи
#export SLOTS=/sys/devices/bone_capemgr.8/slots
#export PINS=/sys/kernel/debug/pinctrl/44e10800.pinmux/pins

2. Kaчват се на ВВВ файловете STIV-GPIO.dts и build-STIV-GPIO и след компилиране се генерира
файла STIV-GPIO-00A0.dtbo който трябва да се копира в /lib/firmware

3. Активиране на овърлейя с команда
#echo STIV-GPIO > $SLOTS
 и се проверява с #cat $SLOTS

4. Активиране на пиновете
root@beaglebone:/sys/class/gpio# ls
export  gpiochip0  gpiochip32  gpiochip64  gpiochip96  unexport
root@beaglebone:/sys/class/gpio# echo 49 > export
root@beaglebone:/sys/class/gpio# echo 115 > export
root@beaglebone:/sys/class/gpio# ls
export   gpio49     gpiochip32  gpiochip96
gpio115  gpiochip0  gpiochip64  unexport

5. Работа с пиновете
cd gpio115
cat direction
echo "out" > direction
cat value
echo "1" > value

**************************
* COPY-PASTE СЛЕД СТАРТ: *
**************************
трябва да се стартира скрипта setEnv

Виж връзката как се осигурява изпълнение със стартирането:
http://tenderlovemaking.com/2014/01/19/enabling-ttyo1-on-beaglebone.html
------------------------------------------------------------------------------------
ИНСТАЛИРАНЕ НА RAM ФАЙЛОВА СИСТЕМА ЗА ОБМЕН С PHP
mkdir /mnt/ramdisk
mount -t tmpfs -o size=4m tmpfs /mnt/ramdisk
touch /mnt/ramdisk/sensors
ln -s /mnt/ramdisk/sensors /www/pages/sensors.html
------------------------------------------------------------------------------------
СПИРАНЕ НА ИЗЛИШНИ СЪРВИСИ И ИНСТАЛИРАНЕ НА lighthttpd и php

http://www.element14.com/community/community/designcenter/single-board-computers/next-gen_beaglebone/blog/2013/11/20/beaglebone-web-server--setup

1.Install Lighttpd
opkg update
opkg install lighttpd lighttpd-module-fastcgi

2.Disable Preloaded Services
systemctl disable cloud9.service
systemctl disable gateone.service
systemctl disable bonescript.service
systemctl disable bonescript.socket
systemctl disable bonescript-autorun.service
systemctl disable avahi-daemon.service
systemctl disable gdm.service
systemctl disable mpd.service

3.restart
shutdown -r now

4. Install PHP
(виж горната уеб връзка)
------------------------------------------------------------------------------------
Sensor: FIRST
Id: 10E36F67-570D-4B82-B4EB-B20831AAAD5B
secret: 494F5931-0A06-45BC-84CB-8F6FD3D67FCD
------------------------------------------------------------------------------------
git

touch README.md
git init
git add README.md
git commit -m "first commit"
git remote add origin https://github.com/farm2440/cloudGrain.git
git push -u origin master

------------------------------------------------------------------------------------
//POST
    //http://qt-project.org/doc/note_revisions/84/235/view
    //QUrl params;
    //params.addQueryItem("key", "value");
    //params.addQueryItem("another_key", "äöutf8-value");

    QUrl site("http://devicewebvarnaservice.azurewebsites.net/RestService/SimpleWriteSingleValueDeviceData");
    QNetworkRequest request(site);
    request.setRawHeader("Content-Type","text/xml;charset=utf-8");

    //if((n%7)==0) manager->post(request, params.encodedQuery());
    if((n%4)==0) manager->post(request, data);

/* //GET
//    if((n%5)==0)   manager->get(QNetworkRequest(QUrl("http://qt-project.org")));
//        if((n%5)==0)   manager->get(QNetworkRequest(QUrl("http://www.etherino.net/index.html")));
  */
----------------------------------------------------------------------------------------
QSerialDevice

QSerialDevice e проекто който след компилиране генерира библиотека и моя проект се свързва към нея.

1. От разархивира се  QSerialDevice_v0.1.0 и от него се взема папката и се копира в текущия проект като поддиректория

2.Отваря се проекта qserialdevice/src/src.pro и се компилира

3. В моя .pro файл се добавя:
DEPENDPATH      += .
INCLUDEPATH     += ./qserialdevice/src
QMAKE_LIBDIR    += ./qserialdevice/src/release
LIBS            += -lqserialdevice

4. За да се ползва класа трябва да е добавено
#include <abstractserial.h>

5. За да се настрои скоростта трябва да се зададе след това и брой битове и четност иначе не се активира зададената скорост
