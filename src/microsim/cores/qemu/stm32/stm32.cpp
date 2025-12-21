/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDebug>
#include <QPainter>
#include <QFileInfo>

#include "stm32.h"
#include <memory>

#include "Afio.h"
#include "itemlibrary.h"
#include "peripheral_factory.h"
#include "stm32pin.h"
#include "qemutwi.h"
#include "stm32Rcm.h"
#include "stm32spi.h"
#include "stm32usart.h"
#include "stm32Gpio.h"
#include "stm32Fmc.h"
#include "simulator.h"
#define tr(str) simulideTr("Stm32",str)

enum armActions{
    STM32_GPIO_OUT = 1,
    STM32_GPIO_CRx,
    STM32_GPIO_IN,
    STM32_ALT_OUT,
    STM32_REMAP
};


Stm32::Stm32( QString type, QString id, QString device )
     : QemuDevice( type, id )
{
    m_area = QRect(-32,-32, 64, 64 );

    m_device = device;
    QString model = device.right( 3 );

    QStringList pkgs = { "T", "C", "R", "V", "Z" };
    uint32_t pkg = pkgs.indexOf( model.mid(1,1) );
    switch( pkg ){
    case 0: m_packageFile = "stm32_dip36.package";  break; // T
    case 1: m_packageFile = "stm32_dip48.package";  break; // C
    case 2: m_packageFile = "stm32_dip64.package";  break; // R
    case 3: m_packageFile = "stm32_dip100.package"; break; // V
    //case 4: m_packageFile = "stm32_dip144.package"; break; // Z

    default: m_packageFile = "stm32_dip36.package";  break;
    }

    QStringList vars = { "4", "6", "8", "B", "C", "D", "E", "F", "G" };
    uint32_t var = vars.indexOf( model.right(1) );
    switch( var ){
        case 0:                                                               // 4
        case 1: { m_portN = 4; m_usartN = 2; m_i2cN = 1; m_spiN = 1; } break; // 6
        case 2:                                                               // 8
        case 3: { m_portN = 5; m_usartN = 3; m_i2cN = 2; m_spiN = 2; } break; // B
        case 4:                                                               // C
        case 5:                                                               // D
        case 6: { m_portN = 7; m_usartN = 5; m_i2cN = 2; m_spiN = 3; } break; // E
        //else if( var == "F" ) m_usartN = 3; m_timerN = 4; m_i2cN = 2;
        //else if( var == "G" ) m_usartN = 3; m_timerN = 4; m_i2cN = 2;
        default: break;
    }

    //TODO set Flash and sram size,and HSI_FREQ,HSE_FREQ,MAX_CPU_FREQ



     target_instr_begin=0;
    //uint64_t target_instr_end=0;
     last_target_time=0;
     target_time=0;
    uint32_t fam = model.left(1).toInt();

    m_model = fam << 16 | pkg << 8 | var;
    //qDebug() << "Stm32::Stm32 model" << device << m_model;


    m_firmware ="";

    createPins();

    m_i2cs.resize( m_i2cN );
    for( int i=0; i<m_i2cN; ++i ) m_i2cs[i] = new QemuTwi( this, "I2C"+QString::number(i), i );

    m_spis.resize( m_spiN );
    for( int i=0; i<m_spiN; ++i ) m_spis[i] = new Stm32Spi( this, "I2C"+QString::number(i), i );

    m_usarts.resize( m_usartN );
    for( int i=0; i<m_usartN; ++i ) m_usarts[i] = new Stm32Usart( this, "Usart"+QString::number(i), i );

    //m_timers.resize( m_timerN );
    //for( int i=0; i<m_timerN; ++i ) m_timers[i] = new QemuTimer( this, "Timer"+QString::number(i), i );
    last_target_time = 0;
    m_psRemainder = 0.0;
    m_stopThread = false;
    m_stopThread = false;
    m_emuThread = std::thread(&Stm32::emuLoop, this);

}
Stm32::~Stm32() {
    m_stopThread = true;
    m_queueCv.notify_all();
    if (m_emuThread.joinable()) {
        m_emuThread.join();
    }
}

void Stm32::stamp()
{
    if( m_i2cN > 0 ) m_i2cs[0]->setPins( m_ports[1].at(6) , m_ports[1].at(7)  );
    if( m_i2cN > 1 ) m_i2cs[1]->setPins( m_ports[1].at(10), m_ports[1].at(11) );

    if( m_spiN > 0 ) m_spis[0]->setPins( m_ports[0].at(7) , m_ports[0].at(6) , m_ports[0].at(5) , m_ports[0].at(4)  );
    if( m_spiN > 1 ) m_spis[1]->setPins( m_ports[1].at(15), m_ports[1].at(14), m_ports[1].at(13), m_ports[1].at(12) );
    if( m_spiN > 2 ) m_spis[2]->setPins( m_ports[1].at(5) , m_ports[1].at(4) , m_ports[1].at(3) , m_ports[0].at(15) );

    if( m_usartN > 0 ) m_usarts[0]->setPins({m_ports[0].at(9) , m_ports[0].at(10)}); // No Remap (TX/PB6, RX/PB7)
    if( m_usartN > 1 ) m_usarts[1]->setPins({m_ports[0].at(2) , m_ports[0].at(3) }); // No remap (CTS/PA0, RTS/PA1, TX/PA2, RX/PA3, CK/PA4), Remap (CTS/PD3, RTS/PD4, TX/PD5, RX/PD6, CK/PD7)
    if( m_usartN > 2 ) m_usarts[2]->setPins({m_ports[1].at(10), m_ports[1].at(11)});
    if( m_usartN > 3 ) m_usarts[3]->setPins({m_ports[2].at(10), m_ports[2].at(11)});
    if( m_usartN > 4 ) m_usarts[4]->setPins({m_ports[2].at(12), m_ports[3].at(2) });

    QemuDevice::stamp();
}
Component * Stm32::construct(QString type, QString id) {
    QString device = Chip::getDevice( id );
    return new Stm32( type, id, device );
}
void Stm32::emuLoop() {
    while (!m_stopThread) {
        EmuTask task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [this]{ return !m_taskQueue.empty() || m_stopThread; });
            if (m_stopThread) break;
            task = m_taskQueue.front();
            m_taskQueue.pop();
        }
        // --- 处理 PC 初始化 ---

        // 处理这一个 task 里的所有指令
        uint64_t remaining = task.count;
        while (remaining > 0 && !m_stopThread) {
            // 分批执行，例如每 5000 条交还一次控制权
            const uint64_t step = std::min(remaining, static_cast<uint64_t>(500000));
            uc_err = uc_emu_start(unicorn_emulator_ptr->get(), target_instr_begin.load(), 0xFFFFFFFF, 0, step);
            target_instr_begin.store(getCurrentPc());
            if (uc_err != UC_ERR_OK) {
                qDebug() << "Unicorn 运行报错:" << uc_strerror(uc_err);
                remaining = 0; // 终止当前任务
                break;
            }
            remaining -= step;

            // 检查此时队列里是否有更紧迫的任务（可选，为了实时性）
            // 如果不想让单次大任务占死线程，可以在这里检查
        }
    }
}
void Stm32::runToTime(const uint64_t time) {
   // qDebug()<<"[Stm32::runToTime]:time:"<<time;
    std::lock_guard<std::mutex> lock(m_queueMutex);
    //qDebug() << "[追踪] 当前时间:" << time << " 与上次差值:" << (time - last_target_time);
    if (last_target_time == 0) {
        last_target_time = time;
        return;
    }

    if (time <= last_target_time) {
   // qDebug() << "[警告] 检测到时间回溯或重置。当前记录时间:" << last_target_time << " 外部传入时间:" << time;
        //  last_target_time = time;
       // m_psRemainder = 0;
        return;
    }

    {
        const uint64_t deltaTime = time - last_target_time;
        const auto rcm_ = dynamic_cast<stm32Rcm::Rcm*>(peripheral_registry->findDevice(RCM_BASE));
        const uint64_t sys_freq = (rcm_ && rcm_->getSysClockFrequency() > 0) ? rcm_->getSysClockFrequency() : 8000000;
        if (sys_freq == 0) {
        qWarning() << "[error] System clock frequency is 0, cannot count instructions";
            return;
        }
        const double ps_per_inst = (1e12 / static_cast<double>(sys_freq)) * 2.0;


        const double total_ps = static_cast<double>(deltaTime) + m_psRemainder;
        const auto count = static_cast<uint64_t>(total_ps / ps_per_inst);

        if (count > 1000000000ULL) {
           // qDebug() << "[严重错误] 检测到异常指令数！可能是时间轴跳变。"
          //           << "差值(deltaTime):" << deltaTime << " 频率:" << sys_freq;
            return;
        }
        if (count > 0) {
            if (!m_taskQueue.empty()) {
                // 如果队列里还有任务，直接把新的指令数加到最后一个任务里
                // 这样 queue.size 永远不会爆炸，减少线程切换开销
                m_taskQueue.back().count += count;
            } else {

                m_taskQueue.push({count});
            }
            m_psRemainder=total_ps-(static_cast<double>(count) * ps_per_inst);
            last_target_time = time;
          //  qDebug() << "任务队列大小:" << m_taskQueue.size() << " 新增指令数:" << count<<"CPU freq:" << sys_freq;
            m_queueCv.notify_one();
        }else {
            qWarning() << "指令数不足 1，等待累计";
        }
    }
    /** const auto rcm_ = dynamic_cast<stm32Rcm::Rcm*>(peripheral_registry->findDevice(RCM_BASE));
    // if (rcm_) {
    //     QElapsedTimer timer;
    //     const uint64_t sys_freq = rcm_->getSysClockFrequency();
    //     double ps_per_inst;
    //     if (sys_freq > 0) {
    //         // 10^12 是 1 秒的皮秒数
    //         ps_per_inst = 1e12 / static_cast<double>(sys_freq);
    //     } else {
    //         //TODO here need do somethings ?
    //         ps_per_inst = 125000.0;
    //     }
    //     //TODO 2.0有点太大，有的时候1.0~1.5，考虑根据固件行为优化这个
    //     ps_per_inst=ps_per_inst*2.0;//mcu实际执行中需要各种等待flash什么的，所以做不到每周期一个指令。这是一个经验值
    //     target_instr_count = calculateInstructionsToExecute(time, last_target_time, ps_per_inst);
    //    timer.start();
    //     if (target_instr_count > 0) {
    //         uc_err = uc_emu_start(unicorn_emulator_ptr->get(), target_instr_begin, 0, 0, target_instr_count);
    //     }
    //    const uint64_t time_use=timer.elapsed();
    //     if (uc_err != UC_ERR_OK) {
    //         //FIXME: TODO somthings
    //         qWarning()<<"void Stm32::runToTime(uint64_t time) uc err"<<uc_strerror(uc_err)<<"pc:"<<Qt::hex<<getCurrentPc()<<Qt::endl;
    //         Simulator::self()->stopSim();
    //     }
    //     qDebug()<<"run to time:"<<time<<"mcu freq"<<sys_freq<<" "<<uc_strerror(uc_err)<<"target_instr_count:"<<target_instr_count<<"use time:"<<time_use<<"ms"<<Qt::endl;
    //     last_target_time = time;
    //     target_instr_begin=getCurrentPc()|1;//// 将 LSB 置 1，告诉 Unicorn 下次从这个地址开始时使用Thumb 模式
    // } else {
    //     qWarning() << "rcm peripheral not found for instruction calculation.";
    // }***/
}
void Stm32::runEvent() {
    while (!event_heap.empty()) {
       // qDebug()<<"stm32 runEvent:"<<Qt::endl;
        ScheduledEvent event = event_heap.top();
       // qDebug()<<"Event:"<<event.scheduled_time<<"type"<<static_cast<int>(event.type)<<Qt::endl;
        event_heap.pop();
        switch (event.type) {
            case EventActionType::GPIO_PIN_SET: {
                setPortState(event.params.gpio_pin_set_data.port,event.params.gpio_pin_set_data.next_state);
            }
                break;
            case EventActionType::GPIO_CONFIG: {
                cofigPort(event.params.gpio_config_data.port,event.params.gpio_config_data.config,event.params.gpio_config_data.shift);
            }
                break;
            case EventActionType::NONE:
            default:
                std::cerr << "[ERROR] Unhandled or Invalid Event Type: " << static_cast<int>(event.type) << std::endl;
                break;
        }
       //  const auto uc=this->unicorn_emulator_ptr->get();
       //  const auto currenPc=getCurrentPc()|1;
       //  target_instr_count=target_instr_count-currenPc;
       // if (target_instr_count > 0) {
       //     uc_emu_start(uc,currenPc,0,0,target_instr_count-currenPc);
       //  }
       if (!event_heap.empty()&&this->eventTime==0) {
            // uc_emu_stop(uc);
            Simulator::self()->addEvent(event_heap.top().scheduled_time,this);
        }
      //  qDebug()<<"剩余evnt数:"<<event_heap.size()<<Qt::endl;
    }
}

void Stm32::createPins()
{
    m_ports.resize( m_portN );
    for( int i=0; i<m_portN; ++i )
        createPort( &m_ports[i], i+1, QString('A'+i), 16 );

    setPackageFile("./data/STM32/"+m_packageFile);
    Chip::setName( m_device );
}

void Stm32::createPort( std::vector<Stm32Pin*>* port, uint8_t number, QString pId, uint8_t n )
{
    for( int i=0; i<n; ++i )
    {
        //qDebug() << "Stm32::createPort" << m_id+"-P"+pId+QString::number(i);
        Stm32Pin* pin = new Stm32Pin( number, i, m_id+"-P"+pId+QString::number(i), this );
        port->emplace_back( pin );
        pin->setVisible( false );
    }
}

void Stm32::doAction()
{
    switch( m_arena->simuAction )
    {
        case STM32_GPIO_OUT:       // Set Output
        {
            uint8_t  port  = m_arena->data8;
            uint16_t state = m_arena->data16;

            if( port >= m_portN ) return;
            if( m_state[port] == state ) return;
            m_state[port] = state;

            //qDebug() << "Stm32::doAction GPIO_OUT Port:"<< port << "State:" << state;

            setPortState( port, state );
        } break;
        case STM32_GPIO_CRx:       // Configure Pins
        {
            uint8_t  port   = m_arena->data8;
            uint8_t  shift  = m_arena->mask8;
            uint32_t config = m_arena->data32;

            if( port < m_portN ) cofigPort( port, config, shift );
        } break;
        case STM32_GPIO_IN:       // Read Inputs
        {
            uint8_t port = m_arena->data8;
            if( port < m_portN ) m_arena->data16 = readInputs( port );
        } break;
        case STM32_ALT_OUT:      // Set Alternate Output
        {
            uint8_t port  = m_arena->data8;
            uint8_t pin   = m_arena->mask8;
            bool    state = (m_arena->data16 > 0);

            if( port < m_portN ) setPinState( port, pin, state );
        } break;
        case STM32_REMAP:       // AFIO Remap
        {
            uint32_t mapr = m_arena->data32;

            uint8_t spi1Map = mapr & 1;
            switch( spi1Map )       // SPI1
            {
                case 0:{        // No remap (NSS/PA4, SCK/PA5, MISO/PA6, MOSI/PA7)
                    m_spis[0]->setPins( m_ports[0].at(7), m_ports[0].at(6), m_ports[0].at(5), m_ports[0].at(4) );
                }break;
                case 1:{        // Remap (NSS/PA15, SCK/PB3, MISO/PB4, MOSI/PB5)
                    m_spis[0]->setPins( m_ports[1].at(5), m_ports[1].at(4), m_ports[1].at(3), m_ports[0].at(15) );
                }break;
            }
            mapr >>= 1;

            switch( mapr & 1 )      // I2C1
            {
                case 0:{        // No remap (SCL/PB6, SDA/PB7)
                    m_i2cs[0]->setSclPin( m_ports[1].at(6) );
                    m_i2cs[0]->setSdaPin( m_ports[1].at(7) );
                }break;
                case 1:{        // Remap (SCL/PB8, SDA/PB9)
                    m_i2cs[0]->setSclPin( m_ports[1].at(8) );
                    m_i2cs[0]->setSdaPin( m_ports[1].at(9) );
                }break;
            }
            mapr >>= 1;

            switch( mapr & 1 )      // USART1
            {
                case 0: m_usarts[0]->setPins({m_ports[0].at(9), m_ports[0].at(10)}); break; //No remap (TX/PA9, RX/PA10)
                case 1: m_usarts[0]->setPins({m_ports[1].at(6), m_ports[1].at(7)});  break;  //Remap (TX/PB6, RX/PB7)
            }
            mapr >>= 1;

            switch( mapr & 1 )      // USART2
            {
                case 0: m_usarts[1]->setPins({m_ports[0].at(3), m_ports[0].at(3)}); break; //No remap (CTS/PA0, RTS/PA1, TX/PA2, RX/PA3, CK/PA4)
                case 1: m_usarts[1]->setPins({m_ports[3].at(5), m_ports[3].at(6)}); break;  //Remap (CTS/PD3, RTS/PD4, TX/PD5, RX/PD6, CK/PD7)
            }
            mapr >>= 1;

            switch( mapr & 0b11 )   // USART3
            {
                case 0: m_usarts[2]->setPins({m_ports[1].at(10), m_ports[1].at(11)}); break; //No remap (TX/PB10, RX/PB11, CK/PB12, CTS/PB13, RTS/PB14)
                case 1: m_usarts[2]->setPins({m_ports[2].at(10), m_ports[2].at(11)}); break; //Partial remap (TX/PC10, RX/PC11, CK/PC12, CTS/PB13, RTS/PB14)
                case 2:                                                               break; //not used
                case 3: m_usarts[2]->setPins({m_ports[3].at(8), m_ports[3].at(9)});   break; // Full remap (TX/PD8, RX/PD9, CK/PD10, CTS/PD11, RTS/PD12)
            }
        }break;
        case SIM_I2C:         // I2C
        {
            uint16_t id = m_arena->data16;

            //qDebug()<< "Stm32::doAction I2C id"<< id<<"data"<<data<<"event"<<event;

            if( id < m_i2cN ) m_i2cs[id]->doAction();
        } break;
        case SIM_SPI:        // SPI
        {
            uint16_t id = m_arena->data16;
            if( id < m_spiN ) m_spis[id]->doAction();
        }break;
        case SIM_USART:      // USART
        {
            uint16_t id = m_arena->data16;
            //qDebug() << "Stm32::doAction SIM_USART:"<< id ;
            if( id < m_usartN ) m_usarts[id]->doAction();
        } break;
        //case SIM_TIMER:
        //{
        //    uint16_t id = m_arena->data16;

        //    if( id < 5 ) m_timers[id]->doAction();
        //    //qDebug() << "Stm32::doAction SIM_TIMER Timer:"<< id << "action:"<< m_arena->simuAction<< "byte:" << data;
        //} break;
        default: qDebug() << "Stm32::doAction Unimplemented"<< m_arena->simuAction;
    }
}
void Stm32::setupDeviceParams() {
  m_mem_params.FLASH_SIZE  = (256 * 1024);
    m_mem_params.SRAM_SIZE   = (64 * 1024);
    m_mem_params.ROM_SIZE  = m_mem_params.FLASH_SIZE;
    m_mem_params.SYS_MEM_START = 0x00020000;
    m_mem_params.SYS_MEM_SIZE  = (1 * 1024);
    m_mem_params.PERIPHERAL_START    = 0x40000000;
    m_mem_params.PERIPHERAL_END      = 0x40023000;
    m_mem_params.PPB_START = 0xE0000000;
}

uint16_t Stm32::readInputs( uint8_t port )
{
    std::vector<Stm32Pin*> ioPort = m_ports[port-1]; //getPort( port );

    uint16_t state = 0;
    for( uint8_t i=0; i<ioPort.size(); ++i )
    {
        Stm32Pin* ioPin = ioPort.at( i );
        if( ioPin->getInpState() ) state |= 1<<i;
    }
    //qDebug() << "Stm32::doAction GPIO_IN"<< port << state;
    return state;
}
bool Stm32::registerPeripheral() {
    peripheral_registry->registerDevice(std::make_unique<stm32Rcm::Rcm>());
    peripheral_registry->registerDevice(std::make_unique<stm32Fmc::Fmc>());
    peripheral_registry->registerDevice(std::make_unique<Afio>());
    peripheral_registry->registerDevice(std::make_unique<stm32Gpio::Gpio>(GPIOA_BASE,"Port A"));
    peripheral_registry->registerDevice(std::make_unique<stm32Gpio::Gpio>(GPIOB_BASE,"Port B"));
    peripheral_registry->registerDevice(std::make_unique<stm32Gpio::Gpio>(GPIOC_BASE,"Port C"));
    peripheral_registry->registerDevice(std::make_unique<stm32Gpio::Gpio>(GPIOD_BASE,"Port D"));
   peripheral_registry->registerDevice(std::make_unique<stm32Gpio::Gpio>(GPIOE_BASE,"Port E"));
    return true;
}
bool Stm32::hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    const auto stm32_instance = static_cast<Stm32*>(user_data);
    const auto rcm_device = dynamic_cast<stm32Rcm::Rcm*>(stm32_instance->peripheral_registry->findDevice(RCM_BASE));
    rcm_device->run_tick(uc, address, size, user_data);
    // 临时打印 PC，看它是如何从 0x8000359 变成 0x4 的
    // qDebug() << "Trace PC:" << Qt::hex << address;
    return QemuDevice::hook_code( uc, address, size, user_data );
}


void Stm32::setPortState( uint8_t port, uint16_t state )
{
   // qDebug() << "Stm32::setPortState,port:"<<port<<"state 0x"<<Qt::hex<<state<<Qt::endl;
    std::vector<Stm32Pin*> ioPort = m_ports[port-1]; //getPort( port );

    for( uint8_t i=0; i<ioPort.size(); ++i )
    {
        Stm32Pin* ioPin = ioPort.at( i );
        ioPin->setPortState( state & (1<<i) );
    }
}

void Stm32::setPinState( uint8_t port, uint8_t pin, bool state )
{
    std::vector<Stm32Pin*> ioPort = m_ports[port]; //getPort( port );

    //qDebug() << "Stm32::setPinState" << port << pin << state;

    Stm32Pin* ioPin = ioPort.at( pin );
    ioPin->setOutState( state );
}

void Stm32::cofigPort( uint8_t port,  uint32_t config, uint8_t shift )
{
    std::vector<Stm32Pin*> ioPort = m_ports[port-1]; //getPort( port );

    //qDebug() << "Stm32::cofigPort GPIO_DIR Port:"<< port <<"config"<<Qt::hex<<config<<"shift"<<shift<<Qt::endl ;

    for( uint8_t i=shift; i<shift+8; ++i )
    {
        Stm32Pin*  ioPin = ioPort.at( i );
        uint8_t cfgShift = i*4;
        uint32_t cfgMask = 0b1111 << cfgShift;
        uint32_t cfgBits = (config & cfgMask) >> cfgShift;

        uint8_t isOutput = cfgBits & 0b0011;  // 0 = Input
       // qDebug()<<"Stm32::cofigPort is output:"<<isOutput<<"pin"<<i ;
        if( isOutput ) // Output
        {
            uint8_t   open = cfgBits & 0b0100;
            pinMode_t pinMode = open ? openCo : output;
            ioPin->setPinMode( pinMode );
            //qDebug()<<"Stm32::cofigPort isOutput pinMode:"<<pinMode;
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
        //qDebug() << "Stm32::addPin"<<id;
        uint n = id.right(2).toInt();
        QString portStr = id.at(1);
        std::vector<Stm32Pin*>* port = nullptr;
        if     ( portStr == "A" ) port = &m_ports[0];
        else if( portStr == "B" ) port = &m_ports[1];
        else if( portStr == "C" ) port = &m_ports[2];
        else if( portStr == "D" ) port = &m_ports[3];
        else if( portStr == "E" ) port = &m_ports[4];

        if( !port ) return nullptr; //new IoPin( angle, QPoint(x, y), m_id+"-"+id, n-1, this, input );

        if( n >= port->size() ) return nullptr;

        pin = port->at( n );
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
