#include "ModMcu.h"

#include <QDir>
#include <QDirIterator>
#include <QLibrary>
#include "ModMcuPin.h"
#include "iopin.h"
#include "simulator.h"
ModMcu::ModMcu(QString type, QString id, QString device) : QemuDevice(type, id) {
  QString const exePath = QCoreApplication::applicationDirPath();
  const QString path = QDir(exePath).absoluteFilePath("data/modMcu/" + type);

  if (QLibrary::isLibrary(path)) {
    qDebug() << Q_FUNC_INFO << ": loading module \"" << path << "\"";
    mylibrary.setFileName(path);
    mylibrary.load();

    m_runToTime = reinterpret_cast<RunToTimeFunc>(mylibrary.resolve("mcu_run_to_time"));
    m_stamp = mylibrary.resolve("mcu_stamp");
    m_initialize = mylibrary.resolve("mcu_initialize");
    m_onPinChanged = reinterpret_cast<PinChangedFunc>(mylibrary.resolve("mcu_on_pin_changed"));
    m_onScheduled = mylibrary.resolve("mcu_on_scheduled_event");
    m_mcuSetUpFunc = reinterpret_cast<McuSetUpHostFunc>(mylibrary.resolve("mcu_setup_host"));
    m_mcuGetConfigFunc=reinterpret_cast<McuGetConfigFunc>(mylibrary.resolve("mcu_get_config"));
    m_mcuSetFirmwareFunc=reinterpret_cast<McuSetFirmwareFunc>(mylibrary.resolve("mcu_set_firmware"));
    if (!m_runToTime || !m_stamp || !m_initialize ||
      !m_onPinChanged || !m_onScheduled || !m_mcuSetUpFunc) {
      qCritical() << "错误: 动态库中缺少必要的导出函数！";

      // 解析失败时，建议把指针全部清空，防止后续逻辑误用
      m_runToTime = nullptr;
      m_stamp = nullptr;
      m_initialize = nullptr;
      m_onPinChanged = nullptr;
      m_onScheduled = nullptr;
      m_mcuSetUpFunc = nullptr;
      m_mcuGetConfigFunc = nullptr;
      m_mcuSetFirmwareFunc = nullptr;
      return;
    }
    const QByteArray ba = type.toUtf8();
    m_cfg=m_mcuGetConfigFunc(ba.constData());

    ModMcuHost host{};
    host.context=this;
    host.addEvent=handleAddEvent;
    host.setPinState=handleSetPinState;
    host.setPortState=handleSetPortState;
    host.configurePins=handleConfigurePins;
    m_mcuSetUpFunc(host);
  } else {
    qWarning() << "not find dll/so file :" + type;
    return;
  }

  m_area = QRect(-32,-32, 64, 64 );
  //TODO get info from dll
  m_device = device;
  m_portN = 7;
  m_usartN = 5;
  m_i2cN = 2;
  m_spiN = 3;

  // m_i2cs.resize( m_i2cN );
  // for( int i=0; i<m_i2cN; ++i ) m_i2cs[i] = new QemuTwi( this, "I2C"+QString::number(i), i );
  //
  // m_spis.resize( m_spiN );
  // for( int i=0; i<m_spiN; ++i ) m_spis[i] = new Stm32Spi( this, "I2C"+QString::number(i), i );
  //
  // m_usarts.resize( m_usartN );
  // for( int i=0; i<m_usartN; ++i ) m_usarts[i] = new Stm32Usart( this, "Usart"+QString::number(i), i );


}
ModMcu::~ModMcu() {
  mylibrary.unload();
}
void ModMcu::stamp() {
  if (m_stamp) {
    m_stamp();
  }
}
void ModMcu::initialize() {
  if (m_initialize) {
    m_initialize();
  }
}
// LibraryItem * ModMcu::libraryItem() {
//   return ;
// }
Component *ModMcu::construct(QString type, QString id) {
  const QString device = getDevice(id);
  return new ModMcu(type, id, device);
}
void ModMcu::runToTime(const uint64_t time) {
  m_runToTime(time);
}
void ModMcu::runEvent() {
  if (m_onScheduled) {
    m_onScheduled();
  }
}
void ModMcu::voltChanged() {
  const bool reset = !m_rstPin->getInpState();
  if (reset) {
    initialize();
  }else {
    stamp();
  }
}
void ModMcu::setFirmware(QString file) {
if (m_mcuSetFirmwareFunc) {
  const auto a=file.toUtf8();
  m_mcuSetFirmwareFunc(a.constData());
}
}
void ModMcu::createPins() {
  m_ports.resize( m_portN );
  for( int i=0; i<m_portN; ++i ) {
    for( int j=0; i<16; ++i )
    {
      auto pin = new ModMcuPin( i+1, j, m_id+"-P"+QString('A'+i)+QString::number(j), this );
      m_ports[i].emplace_back( pin );
      pin->setVisible(false);
    }
  }
  setPackageFile(m_cfg.packageFile);
  setName(m_device);
}
void ModMcu::setPinState(int port, int number, bool state) {
  std::vector<ModMcuPin*> ioPort = m_ports[port];
  ModMcuPin* pin = ioPort.at( number );
  pin->setOutState(state);
}
void ModMcu::setPortState( uint8_t port, uint16_t state )
{
  std::vector<ModMcuPin*> ioPort = m_ports[port-1]; //getPort( port );

  for( uint8_t i=0; i<ioPort.size(); ++i )
  {
    ModMcuPin* ioPin = ioPort.at( i );
    ioPin->setPortState( state & (1<<i) );
  }
}
void ModMcu::cofigPort( uint8_t port,  uint32_t config, uint8_t shift )
{
  std::vector<ModMcuPin*> ioPort = m_ports[port-1]; //getPort( port );

  //qDebug() << "Stm32::cofigPort GPIO_DIR Port:"<< port <<"config"<<Qt::hex<<config<<"shift"<<shift<<Qt::endl ;

  for( uint8_t i=shift; i<shift+8; ++i )
  {
    ModMcuPin*  ioPin = ioPort.at( i );
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

Pin* ModMcu::addPin( QString id, QString type, QString label,
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
    //qDebug() << "Stm32::addPin"<<id;
    uint n1 = id.right(2).toInt();
    QString portStr = id.at(1);
    std::vector<ModMcuPin*>* port = nullptr;
    if     ( portStr == "A" ) port = &m_ports[0];
    else if( portStr == "B" ) port = &m_ports[1];
    else if( portStr == "C" ) port = &m_ports[2];
    else if( portStr == "D" ) port = &m_ports[3];
    else if( portStr == "E" ) port = &m_ports[4];

    if( !port ) return nullptr; //new IoPin( angle, QPoint(x, y), m_id+"-"+id, n-1, this, input );

    if( n1 >= port->size() ) return nullptr;

    pin = port->at( n1 );
    if( !pin ) return nullptr;

    pin->setPos( x, y );
    pin->setPinAngle( angle );
    pin->setVisible( true );
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
