#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <qobject.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <qtimer.h>
#include <termios.h>
#include <qdatetime.h>
#include <unistd.h>

#define TIMER_RX_PERIOD 50 // ms period of the recieving timer
#define RX_BUFFER_SIZE  4096

class SerialPort : public QObject
{
    Q_OBJECT
public:
    enum Parity {PARITY_NONE, PARITY_EVEN, PARITY_ODD };

    SerialPort();
    ~SerialPort();
    int sp_open(const char * port_name); //port_name is the path to the device driver file e.g. /dev/ttyUSB0
    int sp_close();
    bool sp_isOpen(); //return port state

    int sp_write(char * data, int n_bytes); //sends n_bytes bytes of data
    int sp_write(QString str); //sends string
    int	sp_write(char c); //sends one character
    
    
    //read up to rxbuffer_size bytes into rxbuffer array. Retuns the actual number of the recieved bytes
    //to use this function for direct read first rx timer must be disabled
    int sp_read(char * rxbuffer, int rxbuffer_size);
    void sp_clearRxBuffer();
    
    int sp_setPort(unsigned int device_speed,int data_bits, Parity parity, int stop_bits); //set port settings
    int sp_getPortSettings(struct termios * settings); //set port settings using termios structure
    int sp_getPortSpeed();//returns -1 on error, 0 on unrecognised speed otherwise the baudrate
    bool sp_isCanonical(); //returns true if the port is in canonical mode
    int sp_setCanonical(bool canonical, bool ignoreCR, bool mapCRtoNL, bool mapNLtoCR); //set mode
    //hadshaking bits control functions
    void sp_setRTS(bool newState);
    void sp_setDTR(bool newState);
    bool sp_getRTS();
    bool sp_getDTR();
    bool sp_getCTS();
    bool sp_getDSR();
    bool sp_getDCD();
    bool sp_getRI();
    //
    void sp_startRxTimer(); //когато таймера е стартиран се емитира сигнал dataRecieved  и sp_read не може да се ползва
    void sp_stopRxTimer(); //таймера може да се спира. Тогава няма да се емитира сигнал dataRecieved; Приетите данни се вземат с sp_read()
    static unsigned int getBaudByString(char * strBaud); //по низ "9600" връща скороста от тип B9600 .//връща B0 ако не успее
    int hport; //serialport file descriptor (handle). If the port is open for reading/writing the value is not -1

private:
    QTimer *rxTimer; //this timer ticks every  TIMER_RX_PERIOD milliseconds and checks in rxTimerTick() for recieved data. If there is any dataReceived signal is emited
    char buf[RX_BUFFER_SIZE];
    QTime rxTimeout; 
    bool isOpen;
signals:
    void dataReceived(QString str);
    void dataReceived(char * arr, int byteCount);
private slots:
	void rxTimerTick();
};


#endif // SERIALPORT_H
