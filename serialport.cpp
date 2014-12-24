#include "serialport.h"
//-----------------------------------------------------------------------
SerialPort::SerialPort()
{
    hport=-1;
    isOpen = false;
    //memset(&ttyAttrib,0,sizeof(struct termios));

	//setup the recieving timer. FriendlyARM doesn't support QThread
	rxTimer = new QTimer(this);
        //connect(rxTimer, SIGNAL(timeout()), this, SLOT(rxTimerTick()));
        //rxTimer->start(TIMER_RX_PERIOD); //false- повтаря се периодично
}
//-----------------------------------------------------------------------
SerialPort::~SerialPort()
{
    if(hport != -1) close(hport);
}
//-----------------------------------------------------------------------
int SerialPort::sp_open(const char *port_name)
{//opens the port. returns the handle (file descriptor) if successful. otherwise -1
    hport = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY); //open for read and write, use non-blocking i/o
    if(hport>0)
    {        
    	rxTimeout.start(); 
        isOpen=true;
    }
    return hport;
}
//-----------------------------------------------------------------------
int SerialPort::sp_close()
{ //close the port. returns 0 if successful otherwise -1
    int r = close(hport);
    if(r==0) isOpen=false;
    return r;
}
//-----------------------------------------------------------------------
bool SerialPort::sp_isOpen() { return isOpen; }
//-----------------------------------------------------------------------
int SerialPort::sp_write(char *data, int n_bytes)
{//sends n_bytes data. returns number of bytes sent. if error returns -1
    if(hport==-1) return -1;
    return write(hport, data, n_bytes);
}
//-----------------------------------------------------------------------
int SerialPort::sp_write(QString str)
{//sends string. returns number of bytes sent. if error returns -1
    if(hport==-1) return -1;
        return write(hport,str.toLatin1(),str.length());
}
//-----------------------------------------------------------------------
int	SerialPort::sp_write(char c)
{ //sends single char. returns 1 if OK. if error returns -1
    if(hport==-1) return -1;
	return write(hport,&c,1);
}
//-----------------------------------------------------------------------
int SerialPort::sp_setPort(unsigned int device_speed,int data_bits, Parity parity, int stop_bits)
{	//set port speed,data bits, parity, number of stop bits using termios structure bits
    //EXAMPLE CALL: sp_setPort(B9600,8,'N',1);	
    
	/*  DataBits : CS7,CS8
        DeviceSpeed :   B50  B75  B110    B134  B150  B200  B300   B600
                        B1200	  B1800   B2400 B4800 B9600 B19200 B38400
                        B57600    B115200 B230400
    */
///////////////////////////////////////////////////////////////////////////////////////////////
    struct termios options;
    if (tcgetattr( hport,&options) != 0) return -1;

    cfmakeraw(&options);
    /*set data bits*/
    options.c_cflag &= ~CSIZE; //clear
    switch (data_bits)
    {
        case 7:
            options.c_cflag |= CS7; //set
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            return -1;
    }

    /*set checksum mode*/
    switch (parity)
    {
//        case 'n':
//        case 'N':
    case SerialPort::PARITY_NONE:
            options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
            options.c_oflag &= ~OPOST;   /*Output*/ //OPOST: Turn on output processing
            break;
//        case 'o':
//        case 'O':
    case SerialPort::PARITY_ODD:
            options.c_cflag |= (PARODD | PARENB); /* ??? */
            options.c_iflag |= INPCK;             /* Disnable parity checking */
            break;
//        case 'e':
//        case 'E':
    case SerialPort::PARITY_EVEN:
            options.c_cflag |= PARENB;     /* Enable parity */ //PARENB: Enable parity generation and detection.
            options.c_cflag &= ~PARODD;   //PARODD: Use odd parity rather than even parity.
            options.c_iflag |= INPCK;       /* Disnable parity checking */
            break;
/*
        case 'S':
        case 's': //as no parityс
            options.c_cflag &= ~PARENB; //PARENB: Enable parity generation and detection.
            options.c_cflag &= ~CSTOPB; //CSTOPB: Use two stop bits per character, rather than one.
            break;
*/
        default:
            return -1;
    }
    /* set stop bits*/
    switch (stop_bits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB; //CSTOPB: Use two stop bits per character, rather than one.
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            return -1;
    }
    //set speed
    if((cfsetospeed(&options,device_speed)<0) |  (cfsetispeed(&options,device_speed)<0)) return -1;
    return tcsetattr(hport,TCSANOW,&options);
}
//-----------------------------------------------------------------------
int SerialPort::sp_getPortSettings(struct termios * settings)
{
    return tcgetattr(hport,settings);
}
//-----------------------------------------------------------------------
bool SerialPort::sp_isCanonical()
{
struct termios ttyAttrib;	
	//in case of error a false will be returned
	if(hport==-1) return false;
    if(tcgetattr(hport,&ttyAttrib) == -1) return false;
    
    if (ttyAttrib.c_lflag & ICANON) return true;
    else return false;
}
//-----------------------------------------------------------------------
int SerialPort::sp_setCanonical(bool canonical, bool ignoreCR, bool mapCRtoNL, bool mapNLtoCR)
{
	/*
	 * IGNCR    If this bit is set, carriage return characters ('\r') are discarded on input. Discarding carriage return may be useful on terminals that send both carriage return and linefeed when you type the RET key.
	 * ICRNL    If this bit is set and IGNCR is not set, carriage return characters ('\r') received as input are passed to the application as newline characters ('\n').
	 * INLCR    If this bit is set, newline characters ('\n') received as input are passed to the application as carriage return characters ('\r'). 
     */
struct termios ttyAttrib;	
	
	if(hport==-1) return -1;
    if(tcgetattr(hport,&ttyAttrib) == -1) return -1;
    if(canonical)
    {
    	ttyAttrib.c_lflag |= ICANON; //set
    	
    	if(ignoreCR ) ttyAttrib.c_iflag |= IGNCR;
    	else      	  ttyAttrib.c_iflag &= ~IGNCR;
    	//map CR to NL
    	if(mapCRtoNL) ttyAttrib.c_iflag |= ICRNL; 
    	else		  ttyAttrib.c_iflag &= ~ICRNL;
    	//map NL to CR
    	if(mapNLtoCR) ttyAttrib.c_iflag |= INLCR; 
    	else		  ttyAttrib.c_iflag &= ~INLCR; 
    }
    else ttyAttrib.c_lflag &= ~ICANON; //clear
    tcsetattr(hport, TCSANOW, &ttyAttrib); /* set the termio information */
    return 0;
}
//-----------------------------------------------------------------------
bool SerialPort::sp_getRTS()
{
int portStatus;		
	ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
	if(portStatus & TIOCM_RTS) return false;
	else return true;
}
//-----------------------------------------------------------------------
bool SerialPort::sp_getDTR()
{
	int portStatus;		
		ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
		if(portStatus & TIOCM_DTR) return false;
		else return true;		
}
//-----------------------------------------------------------------------
bool SerialPort::sp_getCTS()
{
	int portStatus;		
		ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
		if(portStatus & TIOCM_CTS) return false;
		else return true;
}
//-----------------------------------------------------------------------
bool SerialPort::sp_getDSR()
{
	int portStatus;		
		ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
		if(portStatus & TIOCM_DSR) return false;
		else return true;
}
//-----------------------------------------------------------------------
bool SerialPort::sp_getDCD()
{
	int portStatus;		
		ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
		if(portStatus & TIOCM_CAR) return false;
		else return true;}
//-----------------------------------------------------------------------
bool SerialPort::sp_getRI()
{
	int portStatus;		
		ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
		if(portStatus & TIOCM_RNG) return false;
		else return true;}
//-----------------------------------------------------------------------
void SerialPort::sp_setDTR(bool newState)
{
	/* clear the HUPCL bit. With the HUPCL bit set,
	* when you close the serial port, the Linux serial port driver
	* will drop DTR (assertion level 1, negative RS-232 voltage). By
	* clearing the HUPCL bit, the serial port driver leaves the
	* assertion level of DTR alone when the port is closed.
	*/
struct termios ttyAttrib;	
int portStatus;		
	if(hport==-1) return;
    if(tcgetattr(hport,&ttyAttrib) == -1) return;/* get the termio information */
    ttyAttrib.c_cflag &= ~HUPCL;	/* clear the HUPCL bit */
	tcsetattr(hport, TCSANOW, &ttyAttrib); /* set the termio information */

	ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
	if ( newState )  portStatus &= ~TIOCM_DTR;    /* set the DTR line */	    
	else  portStatus |= TIOCM_DTR;
    ioctl(hport, TIOCMSET, &portStatus); /* set the serial port status */
}
//-----------------------------------------------------------------------
void SerialPort::sp_setRTS(bool newState)
{
	/* clear the HUPCL bit. With the HUPCL bit set,
	* when you close the serial port, the Linux serial port driver
	* will drop DTR (assertion level 1, negative RS-232 voltage). By
	* clearing the HUPCL bit, the serial port driver leaves the
	* assertion level of DTR alone when the port is closed.
	*/
struct termios ttyAttrib;	
int portStatus;		
	if(hport==-1) return;
    if(tcgetattr(hport,&ttyAttrib) == -1) return;/* get the termio information */
    ttyAttrib.c_cflag &= ~HUPCL;	/* clear the HUPCL bit */
	tcsetattr(hport, TCSANOW, &ttyAttrib); /* set the termio information */

	ioctl(hport, TIOCMGET, &portStatus); /* get the serial port status */
	if ( newState )  portStatus &= ~TIOCM_RTS;    /* set the DTR line */	    
	else  portStatus |= TIOCM_RTS;
    ioctl(hport, TIOCMSET, &portStatus); /* set the serial port status */
}
//-----------------------------------------------------------------------
int SerialPort::sp_getPortSpeed()
{//returns -1 on error, 0 on unrecognised speed otherwise the baudrate
struct termios ttyAttrib;
	if(hport==-1) return -1;
    if(tcgetattr(hport,&ttyAttrib) == -1) return -1;
    speed_t s = cfgetospeed(&ttyAttrib);
    switch(s)
    {
        case B50:
            return 50;
        case B75:
            return 75;
        case B110:
            return 110;
        case B134:
            return 134;
        case B150:
            return 150;
        case B200:
            return 200;
        case B300:
            return 300;
        case B600:
            return 600;
        case B1200:
            return 1200;
        case B1800:
            return 1800;
        case B2400:
            return 2400;
        case B4800:
            return 4800;
        case B9600:
            return 9600;
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        case B230400:
            return 230400;
    }
    return 0;
}
//-----------------------------------------------------------------------
void SerialPort::rxTimerTick()
{
	if(hport== -1) return;

    int recievedBytesCount = read(hport,buf,RX_BUFFER_SIZE-1);
    if(recievedBytesCount < 1) return; 
    buf[recievedBytesCount]=0;
    emit dataReceived(QString(buf));    
    emit dataReceived(buf,recievedBytesCount);    
}	
//-----------------------------------------------------------------------
int SerialPort::sp_read(char * rxbuffer, int rxbuffer_size)
{
   int n = read(hport,rxbuffer,rxbuffer_size); 
   return n;
}
//-----------------------------------------------------------------------
void SerialPort::sp_clearRxBuffer()
{
    int n;
    char dummyBuf[10];

    do
    {
        n=read(hport,dummyBuf,10);
    }
    while(n>0);
}
//-----------------------------------------------------------------------
unsigned int SerialPort::getBaudByString(char * strBaud) 
{//по низ "9600" връща скороста от тип B9600
	//ползва се за инициализация на порт ако скоростта е извлечена от текстов файл
	if(strcmp(strBaud,"50")==0) return B50;
	if(strcmp(strBaud,"75")==0) return B75;
	if(strcmp(strBaud,"110")==0) return B110;
	if(strcmp(strBaud,"134")==0) return B134;
	if(strcmp(strBaud,"150")==0) return B150;
	if(strcmp(strBaud,"200")==0) return B200;
	if(strcmp(strBaud,"300")==0) return B300;
	if(strcmp(strBaud,"600")==0) return B600;
	if(strcmp(strBaud,"1200")==0) return B1200;
	if(strcmp(strBaud,"1800")==0) return B1800;
	if(strcmp(strBaud,"2400")==0) return B2400;
	if(strcmp(strBaud,"4800")==0) return B4800;
	if(strcmp(strBaud,"9600")==0) return B9600;
	if(strcmp(strBaud,"19200")==0) return B19200;
	if(strcmp(strBaud,"38400")==0) return B38400;
	if(strcmp(strBaud,"57600")==0) return B57600;
	if(strcmp(strBaud,"115200")==0) return B115200;
	if(strcmp(strBaud,"230400")==0) return B230400;
	return B0; //връща B0 ако не успее	
}
//--------------------------------------------------------------------------------
void SerialPort::sp_startRxTimer()
{
        rxTimer->start(TIMER_RX_PERIOD); // повтаря се периодично
}
void SerialPort::sp_stopRxTimer()
{
	rxTimer->stop();
}
