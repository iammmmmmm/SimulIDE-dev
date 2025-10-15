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
}
Stm32::~Stm32() {
    if (m_SockClient != INVALID_SOCKET) {
        closesocket(m_SockClient);
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

    m_arguments << "qemu-system-arm";

    QStringList extraArgs = m_extraArgs.split(",");
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

void Stm32::setPortState( std::vector<Stm32Pin*>* port, uint16_t state )
{
    for( uint8_t i=0; i<port->size(); ++i )
    {
        Stm32Pin* ioPin = port->at( i );
        ioPin->setPortState( state & (1<<i) );
    }
}
void Stm32::usartReadData(uint8_t data,uint8_t index) {
   //FIXME This method needs to be improved
    qDebug() << "Stm32::usartReadData"<<data<<"index"<<index;
    // 检查套接字是否有效且已连接
    if (m_SockClient == INVALID_SOCKET) {
        qDebug() << "错误：套接字未连接或无效，无法发送数据。";
        qDebug() << "Stm32::Stm32 - 正在初始化网络并连接...";
        int initResult = WSAStartup(MAKEWORD(2, 2), &wsd);
        if (initResult != 0) {
            qDebug() << "WSAStartup 失败！错误码：" << initResult;
            return;
        }
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(4000 + 1);
        serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        m_SockClient = socket(AF_INET, SOCK_STREAM, 0);
        if (m_SockClient == INVALID_SOCKET) {
            qDebug() << "Socket 创建失败！错误码：" << WSAGetLastError();
            WSACleanup();
            return;
        }
        int connectResult = connect(m_SockClient, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (connectResult == SOCKET_ERROR) {
            qDebug() << "Connect 连接失败！错误码：" << WSAGetLastError();
            closesocket(m_SockClient);
            m_SockClient = INVALID_SOCKET; // 标记连接无效
            WSACleanup();
        } else {
            qDebug() << "成功连接到服务器: 127.0.0.1:" << (4000 + 1);
        }
    }

    // 1. 准备要发送的数据
    // 将 uint8_t 变量的地址强制转换为 const char*，并指定长度为 1 字节
    const char* sendBuf = reinterpret_cast<const char*>(&data);

    // 2. 调用 send 函数发送数据
    // 注意：我们将要发送的长度固定为 1 字节
    int bytesSent = send(m_SockClient, sendBuf, 1, 0);

    // 3. 检查发送结果
    if (bytesSent == SOCKET_ERROR) {
        qDebug() << "数据发送失败！错误码：" << WSAGetLastError();
        // 实际应用中，可以在此尝试重新连接
    } else if (bytesSent == 1) {
        qDebug() << "成功发送 1 个字节。";
    } else {
        // TCP 不可能只发送了 0 字节而没有错误，除非连接已关闭
        qDebug() << "警告：发送了 0 字节，连接可能已关闭。";
        closesocket(m_SockClient);
        m_SockClient = INVALID_SOCKET;
    }
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
