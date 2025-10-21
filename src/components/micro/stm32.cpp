/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDebug>
#include <QPainter>
#include <QFileInfo>

#include "stm32.h"
#include "itemlibrary.h"
//#include "iopin.h"
#include "simulator.h"
#include "stm32pin.h"

#define tr(str) simulideTr("Stm32",str)

enum ArmActions{
    ARM_GPIO_OUT = 1,
    ARM_GPIO_CRx,
    ARM_GPIO_IN,
    ARM_UART_TX
};


Component* Stm32::construct( QString type, QString id )
{ return new Stm32( type, id ); }

LibraryItem* Stm32::libraryItem()
{
    return new LibraryItem(
        "Stm32",
        "STM32",
        "ic2.png",
        "Stm32",
        Stm32::construct );
}

Stm32::Stm32( QString type, QString id )
     : QemuDevice( type, id )
{
    m_area = QRect(-32,-32, 64, 64 );

    m_ClkPeriod = 10240000; //6400000; // 6.4 ms
    m_frequency = 72*1000*1000;

    m_executable = "./data/STM32/qemu-system-arm";

    m_firmware ="";

    createPins();

    m_i2c[0].setDevice( this );
    m_i2c[0].setSclPin( m_portB.at(6) );
    m_i2c[0].setSdaPin( m_portB.at(7) );

    m_i2c[1].setDevice( this );
    m_i2c[1].setSclPin( m_portB.at(10) );
    m_i2c[1].setSdaPin( m_portB.at(11) );

    m_usart[0].setPins({m_portA.at(9), m_portA.at(10)}); // Remap (TX/PB6, RX/PB7)
    m_usart[1].setPins({m_portA.at(2), m_portA.at(3)}); // No remap (CTS/PA0, RTS/PA1, TX/PA2, RX/PA3, CK/PA4), Remap (CTS/PD3, RTS/PD4, TX/PD5, RX/PD6, CK/PD7)
    m_usart[2].setPins({m_portB.at(10), m_portB.at(11)});
    m_usart[0].setMcuReadByte([this](uint8_t data) {
        this->usartReadData(data,0);
       });
    m_usart[1].setMcuReadByte([this](uint8_t data) {
       this->usartReadData(data,1);
   });
    m_usart[2].setMcuReadByte([this](uint8_t data) {
       this->usartReadData(data,2);
   });

#ifdef _WIN32
    int initResult = WSAStartup(MAKEWORD(2, 2), &wsd);
    if (initResult != 0) {
        qDebug() << "WSAStartup fail! error code：" << initResult;
        return;
    }
    for (int i = 0; i < 3; ++i) {
        usart_socket_client[i]=INVALID_SOCKET;
    }
#else
    for (int i = 0; i < 3; ++i) {
        usart_socket_client[i]=0;
    }
#endif
    using FinishedSignal = void (QProcess::*)(int exitCode, QProcess::ExitStatus exitStatus);
    FinishedSignal finishedSignal = &QProcess::finished;
    QObject::connect(&m_qemuProcess, finishedSignal,
    [this](int exitCode, QProcess::ExitStatus exitStatus) {
        this->onQemuFinished();
    });
    QObject::connect(&m_qemuProcess, &QProcess::started,[this]() {
        this->onQemuStarted();
    });
}
Stm32::~Stm32() {
    for (const SOCKET s:usart_socket_client) {
        if (s != INVALID_SOCKET) {
            closesocket(s);
        }
    }
    WSACleanup();
}

void Stm32::stamp()
{
    m_usart[0].enable( true );
    m_usart[1].enable( true );
    m_usart[2].enable( true );
    QemuDevice::stamp();
}

void Stm32::createPins()
{
    createPort( &m_portA, "A", 16 );
    createPort( &m_portB, "B", 16 );
    createPort( &m_portC, "C", 16 );
    createPort( &m_portD, "D",  3 );

    setPackageFile("./data/STM32/stm32.package");
}

void Stm32::createPort( std::vector<Stm32Pin*>* port, QString pId, uint8_t n )
{
    for( int i=0; i<n; ++i )
    {
        Stm32Pin* pin = new Stm32Pin( i, m_id+"-P"+pId+QString::number(i), this );
        port->emplace_back( pin );
    }
}

bool Stm32::createArgs()
{
    QFileInfo fi = QFileInfo( m_firmware );

    /*if( fi.size() != 1048576 )
    {
        qDebug() << "Error firmware file size:" << fi.size() << "must be 1048576";
        return false;
    }*/

    m_arena->data32 = m_frequency;

    m_arguments.clear();

    m_arguments << m_shMemKey;          // Shared Memory key

    QStringList extraArgs = m_extraArgs.split(",");

    if (extraArgs[0].isEmpty()) {
        extraArgs[0]="24000000";
    }
    m_arguments << extraArgs[0];
    extraArgs.removeAt(0);

    m_arguments << "qemu-system-arm";

    for( QString arg : extraArgs )
    {
        if( arg.isEmpty() ) continue;
        m_arguments << arg;
    }

    //m_arguments << "-d";
    //m_arguments << "in_asm";

    //m_arguments << "-machine";
    //m_arguments << "help";

    m_arguments << "-M";
    m_arguments << "stm32-f103c8-simul";

    m_arguments << "-drive";
    m_arguments << "file="+m_firmware+",if=pflash,format=raw";

    //m_arguments << "-accel";
    //m_arguments << "tcg,tb-size=100";

    m_arguments << "-icount";
    m_arguments <<"shift=4,align=off,sleep=off";

    //m_arguments << "-kernel";
    //m_arguments << m_firmware;

    m_arguments << "-chardev";
    m_arguments << "socket,id=c0,host=127.0.0.1,port=4000,server=on,wait=off";
    m_arguments << "-serial";
    m_arguments << "chardev:c0";

    m_arguments << "-chardev";
    m_arguments << "socket,id=c1,host=127.0.0.1,port=4001,server=on,wait=off";
    m_arguments << "-serial";
    m_arguments << "chardev:c1";

    m_arguments << "-chardev";
    m_arguments << "socket,id=c2,host=127.0.0.1,port=4002,server=on,wait=off";
    m_arguments << "-serial";
    m_arguments << "chardev:c2";
    return true;
}

void Stm32::doAction()
{
    switch( m_arena->action )
    {
        case ARM_GPIO_OUT:       // Set Output
        {
            uint8_t  port  = m_arena->data8;
            uint16_t state = m_arena->data16;

            if( m_state[port] == state ) return;
            m_state[port] = state;

            //qDebug() << "Stm32::doAction GPIO_OUT Port:"<< port << "State:" << state;
            switch( port ) {
                case 1: setPortState( &m_portA, state ); break;
                case 2: setPortState( &m_portB, state ); break;
                case 3: setPortState( &m_portC, state ); break;
                case 4: setPortState( &m_portD, state ); break;
            }
        } break;
        case ARM_GPIO_CRx:       // Configure Pins
        {
            uint8_t  port   = m_arena->data8;
            uint8_t  shift  = m_arena->mask8;
            uint32_t config = m_arena->data32;

            cofigPort( port, config, shift );
        } break;
        case ARM_GPIO_IN:                  // Read Inputs
        {
            uint8_t  port   = m_arena->data8;
            m_arena->data16 = readInputs( port );
        } break;
        case SIM_USART:
        {
            uint16_t    id = m_arena->data16;
            uint8_t  event = m_arena->data8;
            uint32_t  data = m_arena->data32;

            //qDebug() << "Stm32::doAction SIM_USART Uart:"<< id << "action:"<< event<< "byte:" << data;
            if( id < 3 ) m_usart[id].doAction( event, data );
        } break;
        case SIM_I2C:
        {
            uint16_t    id = m_arena->data16;
            uint8_t   data = m_arena->data32;
            uint8_t  event = m_arena->data8;

            //qDebug()<< "Stm32::doAction I2C id"<< id<<"data"<<data<<"event"<<event;

            if( id < 2 ) m_i2c[id].doAction( event, data );
            break;
        }
        default:
            qDebug() << "Stm32::doAction Unimplemented"<< m_arena->action;
    }
}

uint16_t Stm32::readInputs( uint8_t port )
{
    std::vector<Stm32Pin*>* ioPort = nullptr;

    switch( port ) {
        case 1: ioPort = &m_portA; break;
        case 2: ioPort = &m_portB; break;
        case 3: ioPort = &m_portC; break;
        case 4: ioPort = &m_portD; break;
    }
    if( !ioPort ) return 0;

    uint16_t state = 0;
    for( uint8_t i=0; i<ioPort->size(); ++i )
    {
        Stm32Pin* ioPin = ioPort->at( i );
        if( ioPin->getInpState() ) state |= 1<<i;
    }
    //qDebug() << "Stm32::doAction GPIO_IN"<< port << state;
    return state;
}
void Stm32::onQemuStarted() {
    for (int i = 0; i < 3; ++i) {
        usartSocketInit(i);
    }
    //qDebug()<< "Stm32::onQemuStarted";
}
void Stm32::onQemuFinished() {
#ifdef _WIN32
    for (SOCKET & socket : usart_socket_client) {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }
#else
    for (int & socket : usart_socket_client) {
        if (socket != -1) {
            clos(socket);
            socket = -1;
        }
    }
#endif
    //qDebug()<< "Stm32::onQemuFinished";
}

void Stm32::setPortState( std::vector<Stm32Pin*>* port, uint16_t state )
{
    for( uint8_t i=0; i<port->size(); ++i )
    {
        Stm32Pin* ioPin = port->at( i );
        ioPin->setPortState( state & (1<<i) );
    }
}
void Stm32::usartReadData(uint8_t data,uint8_t index) {
    //qDebug() << "Stm32::usartReadData"<<data<<"index"<<index;

#ifdef _WIN32
    SOCKET s = INVALID_SOCKET;
    if (usart_socket_client[index] == INVALID_SOCKET) {
        usart_socket_client[index]=usartSocketInit(index);
    }
    s = usart_socket_client[index];
    // 1. Prepare data to send
    // Cast the address of a uint8_t variable to const char* and specify a length of 1 byte
    const char* sendBuf = reinterpret_cast<const char*>(&data);

    // 2. Call the send function to send data
    // NOTE: We fix the length to be sent to 1 byte
    int bytesSent = send(s, sendBuf, 1, 0);

    // 3. Check sending result
    if (bytesSent == SOCKET_ERROR) {
        qWarning() << "Data sending failed! Error code:" << WSAGetLastError();
    } else if (bytesSent == 1) {
       // qDebug() << "1 byte sent successfully.";
    } else {
        // TCP It is not possible to send 0 bytes without errors unless the connection is closed
        qWarning() << "Warning: 0 bytes sent, connection may be closed。";
        closesocket(s);
        usart_socket_client[index]=INVALID_SOCKET;
    }
#else
    int s = -1;
    if (usart_socket_client[index] == -1) {
        usart_socket_client[index]=usartSocketInit(index);
    }
    s = usart_socket_client[index];

    const char* sendBuf = reinterpret_cast<const char*>(&data);

    int bytesSent = send(s, sendBuf, 1, 0);

    if (bytesSent == -1) {
        const char *error_message = strerror(errno);
        qWarning() << "Data sending failed! Error code:" <<error_message;
    } else if (bytesSent >= 1) {
        // qDebug() << "1 byte sent successfully.";
    } else {
        const char *error_message = strerror(errno);
        // TCP It is not possible to send 0 bytes without errors unless the connection is closed
        qWarning() << "Warning: 0 bytes sent, connection may be closed。"<<error_message;
        close(s);
        usart_socket_client[index]=-1;
    }
#endif
}
SOCKET Stm32::usartSocketInit(uint8_t index) {
#ifdef _WIN32
    const SOCKET old_s=usart_socket_client[index];
    if (old_s!=INVALID_SOCKET) {
        closesocket(old_s);
        usart_socket_client[index] = INVALID_SOCKET;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4000 + index);
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        qWarning() << "usartSocketInit Socket Creation failed! error code：" << WSAGetLastError();
    }else {
        int connectResult = connect(s, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));
    if (connectResult == SOCKET_ERROR) {
        qWarning() << "usartSocketInit Connect Connection failed! error code：" << WSAGetLastError();
        closesocket(s);
        s = INVALID_SOCKET; // Mark connection is invalid
    } else {
       // qDebug() << "usartSocketInit Successfully connected to the server: 127.0.0.1:" << (4000 + index);
        }
    }
   usart_socket_client[index]=s;
   return s;
#else
    int old_s=usart_socket_client[index];
    if (old_s!=-1) {
        close(old_s);
        usart_socket_client[index]=-1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4000+index);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s == -1) {
        const char *error_message = strerror(errno);
        qWarning() << "usartSocketInit Socket Creation failed! error code：" << error_message;
    }else {
        int connectResult = connect(s, (struct sockaddr*)&serverAddr,sizeof(serverAddr));
        if (connectResult == -1) {
            const char *error_message = strerror(errno);
            qWarning() << "usartSocketInit Connect Connection failed! error code：" << error_message;
            close(s);
            s = -1;
        } else {
            // qDebug() << "usartSocketInit Successfully connected to the server: 127.0.0.1:" << (4000 + index);
        }
    }

    usart_socket_client[index]=s;
    return s;
#endif
}

void Stm32::cofigPort( uint8_t port,  uint32_t config, uint8_t shift )
{
    std::vector<Stm32Pin*>* ioPort = nullptr;

    //qDebug() << "Stm32::doAction GPIO_DIR Port:"<< port << "Directions:" << m_direction;

    switch( port ) {
        case 1: ioPort = &m_portA; break;
        case 2: ioPort = &m_portB; break;
        case 3: ioPort = &m_portC; break;
        case 4: ioPort = &m_portD; break;
    }
    if( !ioPort ) return;

    for( uint8_t i=shift; i<shift+8; ++i )
    {
        Stm32Pin*  ioPin = ioPort->at( i );
        uint8_t cfgShift = i*4;
        uint32_t cfgMask = 0b1111 << cfgShift;
        uint32_t cfgBits = (config & cfgMask) >> cfgShift;

        uint8_t isOutput = cfgBits & 0b0011;  // 0 = Input

        if( isOutput ) // Output
        {
            uint8_t   open = cfgBits & 0b0100;
            pinMode_t pinMode = open ? openCo : output;
            ioPin->setPinMode( pinMode );
        }
        else          // Input
        {
            ioPin->setPinMode( input );
            uint8_t pull = cfgBits & 0b1000;
            ioPin->setPull( pull>0 );
        }

        uint8_t mode = cfgBits & 0b1100; // Analog if CNF0[1:0] == 0
        ioPin->setAnalog( mode==0 );
        /// TODO: if changing to Not Analog // Restore Port State

        uint8_t alternate = cfgBits & 0b1000;
        if( !ioPin->setAlternate( alternate>0 ) )              // If changing to No Alternate
            ioPin->setPortState( (m_state[port] & 1<<i) > 0 ); // Restore Port state
    }
}
      // CNF
        // Input mode:
        //     00: Analog mode
        //     01: Input Floating (reset state)
        //     10: Input with pull-up / pull-down
        //     11: Reserved
        // Output mode:
        // CNF0 0 push-pull
        //      1 Open-drain
        // CNF1 0 General purpose output
        //      1 Alternate function output

Pin* Stm32::addPin( QString id, QString type, QString label,
                   int n, int x, int y, int angle, int length, int space )
{
    IoPin* pin = nullptr;
    //qDebug() << "Stm32::addPin" << id;
    if( type.contains("rst") )
    {
        pin = new IoPin( angle, QPoint(x, y), m_id+"-"+id, n-1, this, input );
        m_rstPin = pin;
        m_rstPin->setOutHighV( 3.3 );
        m_rstPin->setPullup( 1e5 );
        m_rstPin->setInputHighV( 0.65 );
        m_rstPin->setInputLowV( 0.65 );
    }
    else{
        uint n = id.right(2).toInt();
        QString portStr = id.at(1);
        std::vector<Stm32Pin*>* port = nullptr;
        if     ( portStr == "A" ) port = &m_portA;
        else if( portStr == "B" ) port = &m_portB;
        else if( portStr == "C" ) port = &m_portC;
        else if( portStr == "D" ) port = &m_portD;

        if( !port ) return nullptr; //new IoPin( angle, QPoint(x, y), m_id+"-"+id, n-1, this, input );

        if( n > port->size() ) return nullptr;

        pin = port->at( n );
        if( !pin ) return nullptr;

        pin->setPos( x, y );
        pin->setPinAngle( angle );
    }
    QColor color = Qt::black;
    if( !m_isLS ) color = QColor( 250, 250, 200 );

    //if( type.startsWith("inv") ) pin->setInverted( true );

    pin->setLength( length );
    pin->setSpace( space );
    pin->setLabelText( label );
    pin->setLabelColor( color );
    pin->setFlag( QGraphicsItem::ItemStacksBehindParent, true );
    return pin;
}
